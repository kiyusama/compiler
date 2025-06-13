#include <stdio.h>
#include <stdlib.h>
#include <getsym.h>

extern TOKEN tok;
extern FILE *infile;
extern FILE *outfile;
void error(char *s);
void statement(void);

// デバック用プリント
// printf("tok.attr=%d, tok.value=%d\n",tok.attr, tok.value);

// 記号表
typedef struct
{
	int addr;
	char v[MAXIDLEN + 1];
} s_entry;
s_entry s_table[32]; // 記号表の宣言
int s_idx = 0;		 // 記号表登録に使うindex

void compiler(void)
{
	init_getsym();
	getsym();

	if (tok.attr == RWORD && tok.value == PROGRAM)
	{
		getsym();
		if (tok.attr == IDENTIFIER)
		{
			getsym(); // ex11.pでは526を読み込み

			if (tok.attr == SYMBOL && tok.value == SEMICOLON)
			{
				getsym();
				outblock();

				fprintf(outfile, "halt\n"); // 最後にhaltを書く

				if (tok.attr == SYMBOL && tok.value == PERIOD)
				{
					fprintf(stderr, "Parsing Finished. No error found.\n");
				}
				else
					error("At the end, a period is required.");
			}
			else
				error("After program name, a semicolon is needed.");
		}
		else
			error("Program identifier is needed.");
	}
	else
		error("At the first, program declaration is required.");
}

void error(char *s)
{
	fprintf(stderr, "%s\n", s);
	exit(1);
}

//
// Parser
//

void outblock(void)
{
	if (tok.attr == RWORD && tok.value == VAR)
	{
		do
		{
			getsym();
			register_table(tok); // 記号表へ登録

			getsym();
		} while (tok.attr == SYMBOL && tok.value == COMMA);
		while (tok.attr == SYMBOL && tok.value == SEMICOLON)
		{
			getsym();
			if (tok.attr == RWORD && tok.value == PROCEDURE)
			{
				getsym();
				if (tok.attr == IDENTIFIER)
				{
					getsym();
					inblock();
				}
			}
			else
			{
				break;
			}
		}
		statement();
	}
}

void inblock(void)
{
	if (tok.attr == SYMBOL && tok.value == LPAREN)
	{
		do
		{
			getsym();
			if (tok.attr == IDENTIFIER)
			{
				/* code */
			}
			getsym();
		} while (tok.attr == SYMBOL && tok.value == COMMA);
		if (tok.attr == SYMBOL && tok.value == RPAREN)
		{
			getsym();
			if (tok.attr == SYMBOL && tok.value == SEMICOLON)
			{
				getsym();
				if (tok.attr == RWORD && tok.value == VAR)
				{
					do
					{
						getsym();
						if (tok.attr == IDENTIFIER)
						{
							/* code */
						}
						getsym(); // ,or;を読み込み
					} while (tok.attr == SYMBOL && tok.value == COMMA);
				}
				getsym();
				statement();
			}
		}
	}
}

void statement(void)
{
	if (tok.attr == IDENTIFIER)
	{
		int addr = search_table(tok);
		getsym();
		if (tok.attr == SYMBOL && tok.value == BECOMES)
		{
			getsym();
			expression();
			fprintf(outfile, "store %d,r0\n", addr);
		}
		else
		{
			getsym();
			paramlist();
		}
	}
	else if (tok.attr == RWORD && tok.value == BEGIN)
	{
		do
		{
			getsym();
			statement();

		} while (tok.attr == SYMBOL && tok.value == SEMICOLON);
		if (!(tok.attr == RWORD && tok.value == END))
		{
			printf("BEGIN-ENDでエラー\n");
		}

		getsym();
	}
	else if (tok.attr == RWORD && tok.value == IF)
	{
		getsym();
		condition();

		if (tok.attr == RWORD && tok.value == THEN)
		{
			getsym();
			statement();

			if (tok.attr == RWORD && tok.value == ELSE)
			{
				getsym();
				statement();
			}
		}
		else
		{
			printf("ifでエラー\n");
		}
	}
	else if (tok.attr == RWORD && tok.value == WHILE)
	{
		getsym();
		condition();

		if (tok.attr == RWORD && tok.value == DO)
		{
			getsym();
			statement();
		}
		else
		{
			printf("whileでエラー\n");
		}
	}
	else if (tok.attr == RWORD && tok.value == WRITE)
	{
		do
		{
			getsym();
			if (tok.attr == IDENTIFIER)
			{
				fprintf(outfile, "writed %d\n", search_table(tok));
				fprintf(outfile, "loadi r1,'\\n'\n");
				fprintf(outfile, "writec r1\n");
			}
			getsym();
		} while (tok.attr == SYMBOL && tok.value == COMMA);
	}
}

void expression(void)
{
	if (tok.attr == NUMBER)
	{
		fprintf(outfile, "loadi r0,%d\n", tok.value);
	}
	else if (tok.attr == IDENTIFIER)
	{
		fprintf(outfile, "load r0,%d\n", search_table(tok));
	}
	getsym();
	if (tok.value == PLUS || tok.value == MINUS || tok.value == TIMES || tok.value == DIV)
	{
		int op = tok.value;		// 演算子読み込み
		getsym();				// 二つ目の数字読み取り
		if (tok.attr == NUMBER) // 数字の場合
		{
			switch (op)
			{
			case PLUS:
				fprintf(outfile, "addi r0,%d\n", tok.value);
				break;

			case MINUS:
				fprintf(outfile, "subi r0,%d\n", tok.value);
				break;

			case TIMES:
				fprintf(outfile, "muli r0,%d\n", tok.value);
				break;

			case DIV:
				fprintf(outfile, "divi r0,%d\n", tok.value);
				break;

			default:
				printf("演算でエラー\n");
			}
		}
		else if (tok.attr == IDENTIFIER) // 変数の場合
		{
			int addr = search_table(tok);
			switch (op)
			{
			case PLUS:
				fprintf(outfile, "add r0,%d\n", addr);
				break;

			case MINUS:
				fprintf(outfile, "subi r0,%d\n", addr);
				break;

			case TIMES:
				fprintf(outfile, "muli r0,%d\n", addr);
				break;

			case DIV:
				fprintf(outfile, "divi r0,%d\n", addr);
				break;

			default:
				printf("演算でエラー\n");
			}
		}
		getsym();
	}
	return;
}

void paramlist(void)
{
	if (tok.attr == SYMBOL && tok.value == LPAREN)
	{
		do
		{
			getsym();
			expression();
			getsym();
		} while (tok.attr == SYMBOL && tok.value == COMMA);
		if ((tok.attr == SYMBOL && tok.value == RPAREN))
		{
			printf("paramlistでエラー\n");
		}
	}
	getsym();
}

void condition(void)
{
	expression();

	// 比較演算子読み込み
	int co = tok.value;

	getsym();
	expression();

	switch (co)
	{
	case EQL:
		break;
	case NOTEQL:
		break;
	case LESSTHAN:
		break;
	case GRTRTHAN:
		break;
	case LESSEQL:
		break;
	case GRTREQL:
		break;

	default:
		printf("比較でエラー\n");
	}
}

int search_table(TOKEN tok)
{
	for (int i = 0; i < sizeof(s_table) / sizeof(s_entry); i++)
	{
		if (strcmp(tok.charvalue, s_table[i].v) == 0)
		{
			return s_table[i].addr;
		}
	}
	printf("そんな変数はありません: %s\n", tok.charvalue);
}

void register_table(TOKEN tok)
{
	strcpy(s_table[s_idx].v, tok.charvalue);
	s_table[s_idx].addr = s_idx;
	s_idx++;
}