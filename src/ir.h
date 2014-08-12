#ifndef SIMIT_IR_H
#define SIMIT_IR_H

#include <cassert>
#include <string>
#include <list>

#include "types.h"
#include "irvisitors.h"

namespace simit {
namespace internal {

/** The base class of all nodes in the Simit Intermediate Representation
  * (Simit IR) */
class IRNode {
 public:
  IRNode() {}
  IRNode(const std::string &name) : name(name) {}
  virtual ~IRNode();

  void setName(std::string name) { this->name = name; }

  std::string getName() const { return name; }
  virtual void print(std::ostream &os) const = 0;

 protected:
  std::string name;
};

std::ostream &operator<<(std::ostream &os, const IRNode &node);


/** The base IRNode that represents all computed and loaded tensors.  Note that
  * both scalars and elements are considered tensors of order 0. */
class TensorNode : public IRNode {
 public:
  TensorNode(const TensorType *type) : TensorNode("", type) {}
  TensorNode(const std::string &name, const TensorType *type)
      : IRNode(name), type(type) {}
  virtual ~TensorNode();

  virtual void accept(IRVisitor *visitor) = 0;

  const TensorType *getType() const { return type; }
  unsigned int getOrder() const { return type->getOrder(); }
  virtual void print(std::ostream &os) const = 0;

 protected:
  const TensorType *type;
};


/** Represents a \ref Tensor that is defined as a constant or loaded.  Note
  * that it is only possible to define dense tensor literals.  */
class LiteralTensor : public TensorNode {
 public:
  LiteralTensor(TensorType *type, void *data);
  ~LiteralTensor();

  void cast(TensorType *type);
  void accept(IRVisitor *visitor) { visitor->visit(this); };

  void print(std::ostream &os) const;

 private:
  void  *data;
};

/** Index variables describe the iteration domains of tensor operations.*/

/** An index variable describes iteration over an index set.  There are two
  * types of index variables, free index variables and reduction index
  * variables and both types are represented by the IndexVar class.
  *
  * Free index variables simply describe iteration across an index set and do
  * not have a reduction operation (op=FREE).
  *
  * Reduction variables have an associated reduction operation that is
  * performed for each index in the index set.  Examples are SUM, which not
  * surprisingly sums over the index variable (\sum_{i} in latex speak) and
  * product which takes the product over the index variable (\prod_{i}).
  */
class IndexVar {
 public:
  enum Operator {FREE, SUM, PRODUCT};

  IndexVar(const std::string &name, const IndexSetProduct &indexSet,
           Operator op)
      : name(name), indexSet(indexSet), op(op) {}

  std::string getName() const { return name; }
  const IndexSetProduct &getIndexSet() const { return indexSet; }
  Operator getOperator() const { return op; }

 private:
  std::string name;
  IndexSetProduct indexSet;
  Operator op;
};

std::ostream &operator<<(std::ostream &os, const IndexVar &var);


/** A factory for creating index variables with unique names. */
class IndexVarFactory {
 public:
  IndexVarFactory() : nameID(0) {}

  std::shared_ptr<IndexVar> makeFreeVar(const IndexSetProduct &indexSet);
  std::shared_ptr<IndexVar> makeReductionVar(const IndexSetProduct &indexSet,
                                             IndexVar::Operator op);
 private:
  int nameID;
  std::string makeName();
};


/** Instruction that combines one or more tensors.  Merge nodes must be created
  * through the \ref createMerge factory function. */
class IndexExpr : public TensorNode {
 public:
  enum Operator { NEG, ADD, SUB, MUL, DIV };

  typedef std::shared_ptr<internal::IndexVar> IndexVarPtr;

  class IndexedTensor {
   public:
    IndexedTensor(const std::shared_ptr<TensorNode> &tensor,
                  const std::vector<IndexExpr::IndexVarPtr> &indexVariables);
    std::shared_ptr<TensorNode> getTensor() const { return tensor; };
    std::vector<IndexVarPtr> getIndexVariables() const { return indexVariables;}

   private:
    std::shared_ptr<TensorNode> tensor;
    std::vector<IndexVarPtr>    indexVariables;
  };

  IndexExpr(const std::vector<IndexVarPtr> &indexVars,
            Operator op, const std::vector<IndexedTensor> &operands);

  void accept(IRVisitor *visitor) { visitor->visit(this); };

  const std::vector<IndexVarPtr> &getDomain() const;
  IndexExpr::Operator getOperator() const { return op; }
  const std::vector<IndexedTensor> &getOperands() const { return operands; }
  void print(std::ostream &os) const;

 private:
  std::vector<IndexVarPtr> indexVars;
  Operator op;
  std::vector<IndexedTensor> operands;
};


/** Instruction that stores a value to a tensor or an object. */
class Store : public TensorNode {
 public:
  Store(const std::string &name, const TensorType *type)
      : TensorNode(name, type) {}
};


/** Instruction that stores a value to a tensor or an object. */
// TODO: Remove this class (move it into parser and don't inherit from tensor)
class VariableStore : public Store {
 public:
  VariableStore(const std::string &varName, const TensorType *type)
      : Store(varName, type) {}

  void accept(IRVisitor *visitor) { visitor->visit(this); };

  void print(std::ostream &os) const;
};


/** A formal argument to a function. */
class Argument : public TensorNode {
 public:
  Argument(const std::string &name, const TensorType *type)
      : TensorNode(name, type) {}

  void accept(IRVisitor *visitor) { visitor->visit(this); };

  void print(std::ostream &os) const;
};


/** A formal result of a function. */
class Result : public TensorNode {
 public:
  Result(const std::string &name, const TensorType *type)
      : TensorNode(name, type) {}

  void setValue(const std::shared_ptr<TensorNode> &value) {
    this->value = value;
  }
  void accept(IRVisitor *visitor) { visitor->visit(this); };

  const std::shared_ptr<TensorNode> &getValue() const { return value; }
  void print(std::ostream &os) const;

 private:
  std::shared_ptr<TensorNode> value;
};

/** A Simit function. */
class Function : public IRNode {
 public:
  Function(const std::string &name,
           const std::vector<std::shared_ptr<Argument>> &arguments,
           const std::vector<std::shared_ptr<Result>> &results)
      : IRNode(name), arguments(arguments), results(results) {}

  void addStatements(const std::vector<std::shared_ptr<IRNode>> &stmts);


  const std::vector<std::shared_ptr<Argument>> &getArguments() const {
    return arguments;
  }

  const std::vector<std::shared_ptr<Result>> &getResults() const {
    return results;
  }

  const std::vector<std::shared_ptr<IRNode>> &getBody() const {
    return body;
  }

  void print(std::ostream &os) const;

 private:
  std::vector<std::shared_ptr<Argument>> arguments;
  std::vector<std::shared_ptr<Result>> results;
  std::vector<std::shared_ptr<IRNode>> body;
};


/** A Simit test case. Simit test cases can be declared in language comments
  * and can subsequently be picked up by a test framework. */
class Test : public IRNode {
 public:
  Test(std::string name) : IRNode(name) {}

  void print(std::ostream &os) const;

 private:
};

}} // namespace simit::internal

#endif
