#pragma once
#include "tokenizer.h"
#include "file_handling.h"
#include "type_system.h"
#include "assert.h"

class IAST;
class IASTVisitor;
class ASTRoot;
class ASTLiteral;
class ASTBlock;
class ASTInfixExpr;
class ASTPrefixExpr;
class ASTStatement;
class ASTFunctionDefinition;
class ASTFunctionCall;
class ASTVariableDefinition;

enum class ASTType
{
   BracketedExpr,
   PrefixExpr,
   InfixExpr,
   Literal,
   Statement,
   Block,
   FunctionDefinition,
   VariableDefinition,
   FunctionCall,
   Root,
};

class IAST {
protected:
   IAST(ASTType ast_type, PositionRange position) : position(position), ts_data(nullptr) {}
public:
   PositionRange              position;
   std::shared_ptr<TSASTData> ts_data;

   //calls the correct version of the visit on the visitor
   virtual void map(IASTVisitor& visitor) = 0;

   //recurses the traversal on sub elements
   virtual void traverse_inner(IASTVisitor& visitor) = 0;
};

//visitor
class IASTVisitor {
public:
   virtual void map_root(ASTRoot& root);
   virtual void map_literal(ASTLiteral& literal);
   virtual void map_block(ASTBlock& block);
   virtual void map_infix_expr(ASTInfixExpr& infix);
   virtual void map_prefix_expr(ASTPrefixExpr& prefix);
   virtual void map_statement(ASTStatement& statement);
   virtual void map_fn_definition(ASTFunctionDefinition& fn_defn);
   virtual void map_fn_call(ASTFunctionCall& fn_call);
   virtual void map_variable_definition(ASTVariableDefinition& variable_defn);
};

//provides a map_ast that lets you map over any generic ast type.
//NOTE - all of the map_* internally call map_ast for this effect
class IASTGenericVisitor : public IASTVisitor {
public:
   virtual void map_ast(IAST& ast) = 0;

   virtual void map_root(ASTRoot& root);
   virtual void map_literal(ASTLiteral& literal);
   virtual void map_block(ASTBlock& block);
   virtual void map_infix_expr(ASTInfixExpr& infix);
   virtual void map_prefix_expr(ASTPrefixExpr& prefix);
   virtual void map_statement(ASTStatement& statement);
   virtual void map_fn_definition(ASTFunctionDefinition& fn_defn);
   virtual void map_fn_call(ASTFunctionCall& fn_call);
   virtual void map_variable_definition(ASTVariableDefinition& variable_defn);
};

class ASTPrefixExpr : public IAST {
public:
   const Token&          op;
   std::shared_ptr<IAST> expr;

   ASTPrefixExpr(const Token& op, std::shared_ptr<IAST> expr, PositionRange position) :
      op(op), expr(expr), IAST(ASTType::PrefixExpr, position)  {}

   virtual void map(IASTVisitor& visitor)
   {
      visitor.map_prefix_expr(*this);
   }

   virtual void traverse_inner(IASTVisitor& visitor)
   {
      expr->map(visitor);
   }
};

class ASTInfixExpr : public IAST {
public:
   const Token&          op;
   std::shared_ptr<IAST> left;
   std::shared_ptr<IAST> right;

   ASTInfixExpr(std::shared_ptr<IAST> left, const Token& op, std::shared_ptr<IAST> right, PositionRange position) :
      left(left), op(op), right(right), IAST(ASTType::InfixExpr, position) {}

   virtual void map(IASTVisitor& visitor)
   {
      visitor.map_infix_expr(*this);
   }

   virtual void traverse_inner(IASTVisitor& visitor)
   {
      left->map(visitor);
      right->map(visitor);
   }
};

class ASTStatement : public IAST {
public:
   std::shared_ptr<IAST> inner;

   ASTStatement(std::shared_ptr<IAST> inner, PositionRange position) :
      inner(inner), IAST(ASTType::Statement, position) {}


   virtual void map(IASTVisitor& visitor)
   {
      visitor.map_statement(*this);
   }

   virtual void traverse_inner(IASTVisitor& visitor)
   {
      inner->map(visitor);
   }
};

class ASTBlock : public IAST {
public:
   std::vector<std::shared_ptr<IAST> > statements;
   std::shared_ptr<IAST>               return_expr;

   ASTBlock(std::vector<std::shared_ptr<IAST> > statements, std::shared_ptr<IAST> return_expr, PositionRange position) :
      statements(statements), return_expr(return_expr), IAST(ASTType::Block, position) {}

   virtual void map(IASTVisitor& visitor)
   {
      visitor.map_block(*this);
   }

   virtual void traverse_inner(IASTVisitor& visitor)
   {
      for (auto statement: this->statements)
      {
         statement->map(visitor);
      }
   }
};

class ASTLiteral : public IAST {
public:
   const Token& token;

   ASTLiteral(const Token& token, PositionRange position) :
      token(token), IAST(ASTType::Literal, position)  {}

   virtual void map(IASTVisitor& visitor)
   {
      visitor.map_literal(*this);
   }

   virtual void traverse_inner(IASTVisitor& visitor) {}
};

class ASTFunctionDefinition : public IAST {
public:
   typedef std::pair<std::shared_ptr<IAST>, std::shared_ptr<IAST> >   Argument;
   const Token&          fn_name;
   std::vector<Argument> args;

   std::shared_ptr<IAST> body;
   std::shared_ptr<IAST> return_type;

   ASTFunctionDefinition(const Token&          fn_name,
                         std::vector<Argument> args,
                         std::shared_ptr<IAST> return_type,
                         std::shared_ptr<IAST> body,
                         PositionRange         position) :
      fn_name(fn_name),
      args(args),
      return_type(return_type),
      body(body),
      IAST(ASTType::FunctionDefinition, position)
   {
   }

   virtual void map(IASTVisitor& visitor)
   {
      visitor.map_fn_definition(*this);
   }

   virtual void traverse_inner(IASTVisitor& visitor)
   {
      for (auto arg : args)
      {
         arg.first->map(visitor);
         arg.second->map(visitor);
      }
      this->body->map(visitor);

      this->return_type->map(visitor);
   }
};


class ASTFunctionCall : public IAST {
public:

   std::shared_ptr<IAST>               name;
   std::vector<std::shared_ptr<IAST> > params;


   ASTFunctionCall(std::shared_ptr<IAST> name, std::vector<std::shared_ptr<IAST> > params, PositionRange position) :
      name(name), params(params), IAST(ASTType::FunctionCall, position) {}
   virtual void map(IASTVisitor& visitor)
   {
      visitor.map_fn_call(*this);
   }

   virtual void traverse_inner(IASTVisitor& visitor)
   {
      this->name->map(visitor);

      for (auto param: params)
      {
         param->map(visitor);
      }
   }
};

class ASTVariableDefinition : public IAST {
public:
   std::shared_ptr<IAST> name;
   std::shared_ptr<IAST> type;
   std::shared_ptr<IAST> rhs_value;

   ASTVariableDefinition(std::shared_ptr<IAST> name, std::shared_ptr<IAST> type, std::shared_ptr<IAST> rhs_value, PositionRange position) :
      name(name), type(type), rhs_value(rhs_value), IAST(ASTType::VariableDefinition, position) {}

   virtual void map(IASTVisitor& visitor)
   {
      visitor.map_variable_definition(*this);
   }

   virtual void traverse_inner(IASTVisitor& visitor)
   {
      assert(this->name);
      this->name->map(visitor);

      if (this->type)
      {
         this->type->map(visitor);
      }

      if (this->rhs_value)
      {
         this->rhs_value->map(visitor);
      }
   }
};

class ASTRoot : public IAST {
public:
   std::vector<std::shared_ptr<IAST> > children;

   ASTRoot(std::vector<std::shared_ptr<IAST> > children, PositionRange position) :
      IAST(ASTType::Root, position), children(children) {}

   virtual void map(IASTVisitor& visitor)
   {
      visitor.map_root(*this);
   }

   virtual void traverse_inner(IASTVisitor& visitor)
   {
      for (auto child : this->children)
      {
         child->map(visitor);
      }
   }
};

std::shared_ptr<IAST> parse(std::vector<Token>& tokens, std::string& file_data);
