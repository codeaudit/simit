#ifndef SIMIT_SPECIALIZE_GENERIC_FUNCTIONS_H
#define SIMIT_SPECIALIZE_GENERIC_FUNCTIONS_H

#include <list>
#include <string>
#include <unordered_map>

#include "hir.h"
#include "hir_visitor.h"

namespace simit {
namespace hir {

class SpecializeGenericFunctions : public HIRVisitor {
public:
  SpecializeGenericFunctions() : count(0) {}

  void specialize(Program::Ptr);

private:
  virtual void visit(Program::Ptr);
  virtual void visit(FuncDecl::Ptr);
  virtual void visit(CallExpr::Ptr);
  virtual void visit(MapExpr::Ptr);

  void clone(FuncDecl::Ptr, const std::string &);

private:
  typedef std::unordered_map<std::string, FuncDecl::Ptr> FuncMap;
  typedef std::unordered_map<std::string, std::list<FuncDecl::Ptr>> FuncListMap;

  struct FindGenericFuncs : public HIRVisitor {
    FindGenericFuncs(FuncMap &genericFuncs) : genericFuncs(genericFuncs) {}

    void find(Program::Ptr program) { program->accept(this); }

    virtual void visit(FuncDecl::Ptr);

    FuncMap &genericFuncs;
  };

private:
  FuncMap     genericFuncs;
  FuncListMap specializedFuncs;
  unsigned    count;
};

}
}

#endif

