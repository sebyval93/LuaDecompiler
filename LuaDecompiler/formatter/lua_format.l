%{
    #include <formatter.h>
	Formatter& format = Formatter::getInstance();
%}
%%

("--".*) {
    //printf("%s", yytext);
	format.appendStr(yytext);
}

([\[][\[][^\]]*[\]]+([^\]\]][^\]]*[\]]+)*[\]]) {
    format.appendStr(yytext);
}

(\"(\\.|[^"\\])*\") {
    format.appendStr(yytext);
}

";" {
    //printf("%s", yytext);
    format.appendStr(yytext);
    unput('\n');
}

"})))" {
	format.decreaseParan();
	format.decreaseIndent();
    //printf("\n%s}))", format.generateIndent());
    format.appendStr("\n").append(format.generateIndent()).append("}))");
}

"}))" {
	format.decreaseParan();
	format.decreaseIndent();
    //printf("\n%s})", generateIndent());
    format.appendStr("\n").append(format.generateIndent()).append("})");
}

"}))), " {
	format.decreaseParan();
	format.decreaseIndent();
    //printf("\n%s}),\n%s", generateIndent(), generateIndent());
    format.appendStr("\n" + format.generateIndent() + "})),\n" + format.generateIndent());
}

"})), " {
	format.decreaseParan();
	format.decreaseIndent();
    //printf("\n%s}),\n%s", generateIndent(), generateIndent());
    format.appendStr("\n" + format.generateIndent() + "}),\n" + format.generateIndent());
}

"}))," {
	format.decreaseParan();
	format.decreaseIndent();
    //printf("\n%s}),\n%s", generateIndent(), generateIndent());
    format.appendStr("\n" + format.generateIndent() + "}),\n" + format.generateIndent());
}

"({" {
    unput('{');
}

"{ " {
    unput('{');
}

"{" {
    //printf("\n%s%s", generateIndent(), yytext);
    format.appendStr("\n" + format.generateIndent() + yytext);
    format.increaseIndent();
	format.setWithinTable(true);
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
	format.decreaseIndent();
    //printf("\n%s%s", generateIndent(), yytext);
    format.appendStr("\n" + format.generateIndent() + yytext);
	format.setWithinTable(false);
    unput('\n');
}

"}" {
    format.decreaseIndent();
    //printf("\n%s%s", generateIndent(), yytext);
    format.appendStr("\n" + format.generateIndent() + yytext);
    format.setWithinTable(false);
    //unput('\n');
}

"(" {
	format.increaseParan();
    //printf("%s", yytext);
    format.appendStr(yytext);
}

")" {
    format.decreaseParan();
    //printf("%s", yytext);
    format.appendStr(yytext);
}

", " {
    if (format.isWithinTable())
    {
        unput(',');
    }
    else
        format.appendStr(yytext);
        //printf("%s", yytext);
}

"," {
    if (format.isWithinTable())
    {
        //printf("%s", yytext);
        format.appendStr(yytext);
        unput('\n');
    }
    else
        format.appendStr(yytext);
        //printf("%s", yytext);
}

^(function.*) {
    format.increaseIndent();
    //printf("%s", yytext);
    format.appendStr(yytext);
}

^(if.*(then)) {
    format.increaseIndent();
    //printf("%s", yytext);
    format.appendStr(yytext);
}

^(end.*) {
    format.decreaseIndent();
    // because regex is hard :/
    // deletes the extra tab before the "end" keyword
    //fseek(outputFile, -1, SEEK_CUR);
    //printf("%s\n", yytext);
	format.removeLastChar();
    format.appendStr(yytext).append("\n");
}

(\n) {
    //printf("\n%s", generateIndent());
    format.appendStr("\n").append(format.generateIndent());
}

. {
    //printf("%s", yytext);
    format.appendStr(yytext);
}
%%
