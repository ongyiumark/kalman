#include "Expressions/syntax-expressions.h"

#include <iostream>
using namespace Syntax;
using namespace Objects;
using namespace Diagnostics;

// This turns the tokens into a tree of syntax. 
Parser::Parser(std::string& text, bool show_return) : _position(0), _show_return(show_return)
{
    Lexer lexer(text);
    SyntaxToken token;
    do
    {
        token = lexer.lex();
        switch(token.kind())
        {
            case SyntaxKind::BadToken:
            case SyntaxKind::WhitespaceToken:
            case SyntaxKind::CommentToken:
                break;
            default:
                _tokens.push_back(token);
        }
    } while (token.kind() != SyntaxKind::EndOfFileToken);
}

SyntaxToken Parser::peek(int offset) const
{  
    unsigned int index = _position+offset;
    if (index >= _tokens.size()) 
        index = _tokens.size()-1;
    
    return _tokens[index];
}

SyntaxToken Parser::current() const
{
    return peek(0);
}

SyntaxToken Parser::look_ahead() const
{
    return peek(1);
}

SyntaxToken Parser::next_token()
{
    SyntaxToken curr = current();
    _position++;
    return curr;
}

// Inserts the relevant token, but reports an error.
SyntaxToken Parser::match_token(SyntaxKind kind)
{
    if (current().kind() == kind)
        return next_token();

    Diagnostics::DiagnosticBag::report_unexpected_token(kind_to_string(current().kind()), 
        kind_to_string(kind), current().get_pos());
    return SyntaxToken(kind, current().get_pos(), "\0");
}

// Refer to grammar.txt for the full summary of syntax.
SyntaxNode* Parser::parse()
{
    SyntaxNode* program = parse_program();
    match_token(SyntaxKind::EndOfFileToken);
    return program;
}

SyntaxNode* Parser::parse_program(bool sub_program)
{
    std::vector<SyntaxNode*> program_seq;
    Position start = current().get_pos();
    // It's a sub-program when it's enclosed in curly braces.
    if (sub_program)
    {
        while(current().kind() != SyntaxKind::RCurlyToken)
        {
            program_seq.push_back(parse_statement());
            switch(current().kind())
            {
                case SyntaxKind::RParenToken:
                case SyntaxKind::RSquareToken:
                case SyntaxKind::CommaToken:
                {
                    Diagnostics::DiagnosticBag::report_unexpected_token(kind_to_string(current().kind()), 
                        kind_to_string(SyntaxKind::SemicolonToken), current().get_pos());
                    next_token();
                }
                case SyntaxKind::EndOfFileToken:
                {
                    match_token(SyntaxKind::RCurlyToken);
                    return new SequenceExpressionSyntax(program_seq, 
                        Position(start.ln, start.col, start.start, current().get_pos().end), 
                        _show_return); 
                }
                default:
                    break;
            }
        }
        next_token();
        return new SequenceExpressionSyntax(program_seq, 
            Position(start.ln, start.col, start.start, current().get_pos().end),  
            _show_return); 
    }

    while(current().kind() != SyntaxKind::EndOfFileToken)
    {
        program_seq.push_back(parse_statement());
        switch(current().kind())
        {
            case SyntaxKind::RParenToken:
            case SyntaxKind::RSquareToken:
            case SyntaxKind::RCurlyToken:
            case SyntaxKind::CommaToken:
            {
                Diagnostics::DiagnosticBag::report_unexpected_token(kind_to_string(current().kind()), 
                    kind_to_string(SyntaxKind::SemicolonToken), current().get_pos());
                next_token();
            }
            default:
                break;
        }
    }
        
    return new SequenceExpressionSyntax(program_seq,
        Position(start.ln, start.col, start.start, current().get_pos().end), 
        _show_return);
}

SyntaxNode* Parser::parse_statement()
{
    switch(current().kind())
    {
        case SyntaxKind::LCurlyToken:
        {
            next_token();
            SyntaxNode* sub_program = parse_program(true);
            return sub_program;
        }
        case SyntaxKind::IfKeyword:
        {
            std::vector<SyntaxNode*> conditions;
            std::vector<SyntaxNode*> bodies;
            SyntaxNode* else_body = nullptr;
            Position start = current().get_pos();
            next_token();
            match_token(SyntaxKind::LParenToken);
            SyntaxNode* condition = parse_expression();
            match_token(SyntaxKind::RParenToken);
            SyntaxNode* body = parse_statement();

            conditions.push_back(condition);
            bodies.push_back(body);

            while(current().kind() == SyntaxKind::ElifKeyword)
            {
                next_token();

                match_token(SyntaxKind::LParenToken);
                condition = parse_expression();
                match_token(SyntaxKind::RParenToken);
                body = parse_statement();

                conditions.push_back(condition);
                bodies.push_back(body);
            }

            if (current().kind() == SyntaxKind::ElseKeyword)
            {
                next_token();
                else_body = parse_statement();
            }

            return new IfExpressionSyntax(conditions, bodies, else_body, 
                Position(start.ln, start.col, start.start, current().get_pos().end));
        }
        case SyntaxKind::WhileKeyword:
        {
            Position start = current().get_pos();
            next_token();
            match_token(SyntaxKind::LParenToken);
            SyntaxNode* condition = parse_expression();
            match_token(SyntaxKind::RParenToken);

            SyntaxNode* body = parse_statement();
            return new WhileExpressionSyntax(condition, body, Position(start.ln, start.col, start.start, current().get_pos().end));
        }
        case SyntaxKind::ForKeyword:
        {
            Position start = current().get_pos();
            next_token();
            match_token(SyntaxKind::LParenToken);
            SyntaxNode* init = parse_expression();
            match_token(SyntaxKind::SemicolonToken);
            SyntaxNode* condition = parse_expression();
            match_token(SyntaxKind::SemicolonToken);
            SyntaxNode* update = parse_expression();
            match_token(SyntaxKind::RParenToken);

            SyntaxNode* body = parse_statement();
            return new ForExpressionSyntax(init, condition, update, body, Position(start.ln, start.col, start.start, current().get_pos().end));
        }
        case SyntaxKind::DefineFunctionKeyword:
        {
            Position start = current().get_pos();
            next_token();
            SyntaxToken identifier = match_token(SyntaxKind::IdentifierToken);
            match_token(SyntaxKind::LParenToken);
            std::vector<SyntaxToken> arg_names;

            if (current().kind() != SyntaxKind::RParenToken)
            {
                SyntaxToken arg_name = match_token(SyntaxKind::IdentifierToken);
                arg_names.push_back(arg_name);
                while(current().kind() == SyntaxKind::CommaToken)
                {
                    next_token();
                    arg_name = match_token(SyntaxKind::IdentifierToken);
                    arg_names.push_back(arg_name);
                }
            }
            
            match_token(SyntaxKind::RParenToken);
            SyntaxNode* body = parse_statement();
            return new FuncDefineExpressionSyntax(identifier, arg_names, body,
                Position(start.ln, start.col, start.start, current().get_pos().end));
        }
        case SyntaxKind::ReturnKeyword:
        {
            next_token();
            if (current().kind() == SyntaxKind::SemicolonToken)
            {
                next_token();
                return new ReturnExpressionSyntax(nullptr);
            }

            SyntaxNode* expression = parse_expression();
            match_token(SyntaxKind::SemicolonToken);
            return new ReturnExpressionSyntax(expression);
        }
        case SyntaxKind::BreakKeyword:
        {
            next_token();
            match_token(SyntaxKind::SemicolonToken);
            return new BreakExpressionSyntax();
        }
        case SyntaxKind::ContinueKeyword:
        {
            next_token();
            match_token(SyntaxKind::SemicolonToken);
            return new ContinueExpressionSyntax();
        }
        case SyntaxKind::SemicolonToken:
        {
            next_token();
            return new NoneExpressionSyntax();
        }
        default:
        {
            SyntaxNode* expression = parse_expression();
            match_token(SyntaxKind::SemicolonToken);
            return expression;
        }
    }
}

SyntaxNode* Parser::parse_expression(int precedence)
{
    SyntaxNode* left;
    int unary_precedence = SyntaxFacts::get_unary_precedence(current().kind());
    if (unary_precedence && unary_precedence >= precedence)
    {
        SyntaxToken op_token = next_token();
        SyntaxNode* expression = parse_expression(unary_precedence);
        left = new UnaryExpressionSyntax(op_token, expression, 
            Position(op_token.get_pos().ln, op_token.get_pos().col, op_token.get_pos().start, expression->get_pos().end));
    }
    else 
    {
        switch(current().kind())
        {
            case SyntaxKind::IntegerKeyword:
            case SyntaxKind::DoubleKeyword:
            case SyntaxKind::BooleanKeyword:
            case SyntaxKind::ListKeyword:
            case SyntaxKind::FunctionKeyword:
            case SyntaxKind::StringKeyword:
            {
                Position start = current().get_pos();
                SyntaxToken var_keyword = next_token();
                SyntaxToken identifier = match_token(SyntaxKind::IdentifierToken);
                SyntaxNode* var_decl = new VarDeclareExpressionSyntax(var_keyword, identifier,
                    Position(start.ln, start.col, start.start, current().get_pos().end));
                if (current().kind() == SyntaxKind::SemicolonToken)
                    return var_decl;
                
                match_token(SyntaxKind::EqualsToken);
                SyntaxNode* expression = parse_expression(precedence);
                SyntaxNode* var_ass = new VarAssignExpressionSyntax(identifier, expression,
                    Position(start.ln, start.col, start.start, current().get_pos().end));
                std::vector<SyntaxNode*> seq = {var_decl, var_ass};
                return new SequenceExpressionSyntax(seq, Position(start.ln, start.col, start.start, current().get_pos().end));
            }
            case SyntaxKind::IdentifierToken:
            {
                if (look_ahead().kind() == SyntaxKind::EqualsToken)
                {
                    Position start = current().get_pos();
                    SyntaxToken identifier = next_token();
                    next_token();
                    SyntaxNode* expression = parse_expression(precedence);
                    return new VarAssignExpressionSyntax(identifier, expression,
                        Position(start.ln, start.col, start.start, current().get_pos().end));
                }
                break;
            }  
            default:
                break;      
        }
        left = parse_molecule();
    }

    while(true)
    {
        int binary_precedence = SyntaxFacts::get_binary_precedence(current().kind());
        if (!binary_precedence || binary_precedence <= precedence)
            break;
        
        SyntaxToken op_token = next_token();
        SyntaxNode* right = parse_expression(binary_precedence);

        Position left_pos = left->get_pos();
        Position right_pos = right->get_pos();
        left = new BinaryExpressionSyntax(left, op_token, right, 
            Position(left_pos.ln, left_pos.col, left_pos.start, right_pos.end));
    }
    return left;
}

SyntaxNode* Parser::parse_molecule()
{
    SyntaxNode* left = parse_atom();
    switch(current().kind())
    {
        case SyntaxKind::LSquareToken:
        {
            while(current().kind() == SyntaxKind::LSquareToken)
            {
                Position start = current().get_pos();
                next_token();
                SyntaxNode* right = parse_expression();
                match_token(SyntaxKind::RSquareToken);
                left = new IndexExpressionSyntax(left, right,
                    Position(start.ln, start.col, start.start, current().get_pos().end));
            }
            break;
        }     
        default:
            break;
    }
    return left;   
}

SyntaxNode* Parser::parse_atom()
{
    switch(current().kind())
    {
        case SyntaxKind::IntegerToken:
        {
            SyntaxToken literal_token = next_token();
            std::istringstream is(literal_token.get_text());
            long long x;
            if (is >> x) return new LiteralExpressionSyntax(new Integer(x), literal_token.get_pos());
            
            return new LiteralExpressionSyntax(nullptr, Position());
        }
        case SyntaxKind::StringToken:
        {
            SyntaxToken literal_token = next_token();
            return new LiteralExpressionSyntax(new String(literal_token.get_text()), literal_token.get_pos());
        }
        case SyntaxKind::DoubleToken:
        {
            SyntaxToken literal_token = next_token();
            std::istringstream is(literal_token.get_text());
            long double x;
            if (is >> x) return new LiteralExpressionSyntax(new Double(x), literal_token.get_pos());
            
            return new LiteralExpressionSyntax(nullptr, Position());
        }
        case SyntaxKind::TrueKeyword:
        case SyntaxKind::FalseKeyword:
        {
            SyntaxToken keyword = next_token();
            bool value = keyword.kind() == SyntaxKind::TrueKeyword;
            return new LiteralExpressionSyntax(new Boolean(value), keyword.get_pos());
        }
        case SyntaxKind::LParenToken:
        {
            next_token();
            SyntaxNode* expression = parse_expression();
            match_token(SyntaxKind::RParenToken);
            return expression;
        }
        case SyntaxKind::LSquareToken:
        {
            Position start = current().get_pos();
            next_token();
            std::vector<SyntaxNode*> elements;
            if (current().kind() == SyntaxKind::RSquareToken)
            {
                next_token();
                return new SequenceExpressionSyntax(elements, Position(start.ln, start.col, start.start, current().get_pos().end), true);
            }

            SyntaxNode* expression = parse_expression();
            elements.push_back(expression);

            while(current().kind() == SyntaxKind::CommaToken)
            {
                next_token();
                expression = parse_expression();
                elements.push_back(expression);
            }
            match_token(SyntaxKind::RSquareToken);
            return new SequenceExpressionSyntax(elements, Position(start.ln, start.col, start.start, current().get_pos().end), true);
        }
        case SyntaxKind::PrintFunction:
        case SyntaxKind::InputFunction:
        case SyntaxKind::SplitFunction:
        case SyntaxKind::SizeFunction:
        case SyntaxKind::TypeFunction:
        case SyntaxKind::ToBoolFunction:
        case SyntaxKind::ToIntFunction:
        case SyntaxKind::ToDoubleFunction:
        case SyntaxKind::ToStringFunction:
        case SyntaxKind::SetIndexFunction:
        {
            Position start = current().get_pos();
            SyntaxToken identifier = next_token();
            match_token(SyntaxKind::LParenToken);
            std::vector<SyntaxNode*> args;
            if (current().kind() == SyntaxKind::RParenToken)
            {
                next_token();
                return new FuncCallExpressionSyntax(identifier, args,
                    Position(start.ln, start.col, start.start, current().get_pos().end));
            }

            SyntaxNode* expression = parse_expression();
            args.push_back(expression);

            while(current().kind() == SyntaxKind::CommaToken)
            {
                next_token();
                expression = parse_expression();
                args.push_back(expression);
            }
            match_token(SyntaxKind::RParenToken);
            return new FuncCallExpressionSyntax(identifier, args,
                Position(start.ln, start.col, start.start, current().get_pos().end));
        }
        default:
        {
            Position start = current().get_pos();
            SyntaxToken identifier = match_token(SyntaxKind::IdentifierToken);
            if (current().kind() == SyntaxKind::LParenToken)
            {
                next_token();
                std::vector<SyntaxNode*> args;
                if (current().kind() == SyntaxKind::RParenToken)
                {
                    next_token();
                    return new FuncCallExpressionSyntax(identifier, args,
                        Position(start.ln, start.col, start.start, current().get_pos().end));
                }

                SyntaxNode* expression = parse_expression();
                args.push_back(expression);

                while(current().kind() == SyntaxKind::CommaToken)
                {
                    next_token();
                    expression = parse_expression();
                    args.push_back(expression);
                }
                match_token(SyntaxKind::RParenToken);
                return new FuncCallExpressionSyntax(identifier, args,
                    Position(start.ln, start.col, start.start, current().get_pos().end));
            }
            return new VarAccessExpressionSyntax(identifier,
                Position(start.ln, start.col, start.start, current().get_pos().end));
        }
    }
}