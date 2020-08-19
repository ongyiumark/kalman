#include "syntax.h"

bool CodeAnalysis::is_digit(char c)
{
    return '0' <= c && c <= '9';
}

std::string CodeAnalysis::syntax_kind_to_string(const SyntaxKind& kind)
{
    const char* s = 0;
#define PROCESS_VAL(p) case(p): s = #p; break;
    switch(kind)
    {
        PROCESS_VAL(SyntaxKind::NumberToken);
        PROCESS_VAL(SyntaxKind::WhitespaceToken);
        PROCESS_VAL(SyntaxKind::PlusToken);
        PROCESS_VAL(SyntaxKind::MinusToken);
        PROCESS_VAL(SyntaxKind::StarToken);
        PROCESS_VAL(SyntaxKind::SlashToken);
        PROCESS_VAL(SyntaxKind::LParenToken);
        PROCESS_VAL(SyntaxKind::RParenToken);
        PROCESS_VAL(SyntaxKind::BadToken);
        PROCESS_VAL(SyntaxKind::EndOfFileToken);
        PROCESS_VAL(SyntaxKind::LiteralExpression);
        PROCESS_VAL(SyntaxKind::BinaryExpression);
        PROCESS_VAL(SyntaxKind::UnaryExpression);
        PROCESS_VAL(SyntaxKind::ParenExpression);
    }
#undef PROCESS_VAL
    return s;
}

std::ostream& CodeAnalysis::operator<<(std::ostream& os, const SyntaxKind& kind)
{
    os << syntax_kind_to_string(kind);
    return os;
}

void CodeAnalysis::pretty_print(SyntaxNode* node, std::string indent, bool is_last)
{
    std::string marker = is_last ? "|--" : "|--";

    std::cout << indent << marker;
    std::cout << node->get_kind(); 
    switch(node->get_kind())
    {
        case SyntaxKind::NumberToken:
        {
            std::cout << " ";
            SyntaxToken* token = (SyntaxToken*)node;
            if (token->get_value().type().name()[0] == 'i')
                std::cout << std::any_cast<int>(token->get_value());
            break;
        }
    }

    std::cout << std::endl;
    indent += is_last ? "   " : "|  ";  

    SyntaxNode* last_child = node;
    int n = node->get_children_size();
    if (n) last_child = node->get_child(n-1);
    
    for (int i = 0; i < n; i++)
        pretty_print(node->get_child(i), indent, node->get_child(i) == last_child);
}