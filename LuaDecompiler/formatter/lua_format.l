%{
    #include <formatter.h>
	Formatter& format = Formatter::getInstance();
%}
%%

("--".*) {
    format.comment(std::string(yytext), false);
}

([\[][\[][^\]]*[\]]+) {
	// TODO: handle nested [[ [[ ]] ]]
    format.comment(std::string(yytext), true);
}

(\"(\\.|[^"\\])*\") {
    format.string(std::string(yytext));
}

";" {
    format.semicolon(std::string(yytext));
    unput('\n');
}

("})"[)]+[,][ ])  {
    format.tableEnd(std::string(yytext));
}

("})"[)]+[,]) {
    format.tableEnd(std::string(yytext));
}

("})"[)]+) {
    format.tableEnd(std::string(yytext));
}

"({" {
    unput('{');
}

"{ " {
    unput('{');
}

"{" {
    format.tableStart(std::string(yytext));
    unput('\n');
}

"})" {
    unput('}');
}

"}, " {
    unput(',');
    unput('}');
}

"}," {
    format.tableEnd(std::string(yytext));
    unput('\n');
}

"}" {
    format.tableEnd(std::string(yytext));
}

", " {
    if (format.isWithinTable())
    {
        unput(',');
    }
    else
        format.comma(std::string(yytext));
}

"," {
	format.comma(std::string(yytext));
    if (format.isWithinTable())
    {
        unput('\n');
    }
}

^(function.*) {
    format.functionStart(std::string(yytext));
}

^(if.*(then)) {
    format.conditionStart(std::string(yytext));
}

^(for.*(do)) {
	format.forLoopStart(std::string(yytext));
}

^(end.*) {
    format.blockEnd(std::string(yytext));
}

(\n) {
    format.newLine(std::string(yytext));
}

. {
    format.anyChar(std::string(yytext));
}
%%
