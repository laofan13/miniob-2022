
%{

#include "sql/parser/parse_defs.h"
#include "sql/parser/yacc_sql.tab.h"
#include "sql/parser/lex.yy.h"
// #include "common/log/log.h" // 包含C++中的头文件

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef struct ParserContext {
  Query * ssql;
  Selects selection;
  size_t select_length;
  size_t condition_length;
  size_t from_length;
  size_t value_length;
  size_t record_length;
  Value values[MAX_NUM];
  Condition conditions[MAX_NUM];
  CompOp comp;
  AggrType aggrOp;
  int nullable;
  char id[MAX_NUM];
} ParserContext;

//获取子串
char *substr(const char *s,int n1,int n2)/*从s中提取下标为n1~n2的字符组成一个新字符串，然后返回这个新串的首地址*/
{
  char *sp = malloc(sizeof(char) * (n2 - n1 + 2));
  int i, j = 0;
  for (i = n1; i <= n2; i++) {
    sp[j++] = s[i];
  }
  sp[j] = 0;
  return sp;
}

void yyerror(yyscan_t scanner, const char *str)
{
  ParserContext *context = (ParserContext *)(yyget_extra(scanner));
  query_reset(context->ssql);
  context->ssql->flag = SCF_ERROR;
  context->condition_length = 0;
  context->from_length = 0;
  context->select_length = 0;
  context->value_length = 0;
  context->record_length = 0;
  context->ssql->sstr.insertion.record_num = 0;
  context->nullable = 0;
  printf("parse sql failed. error=%s", str);
}

ParserContext *get_context(yyscan_t scanner)
{
  return (ParserContext *)yyget_extra(scanner);
}

#define CONTEXT get_context(scanner)

%}

%define api.pure full
%lex-param { yyscan_t scanner }
%parse-param { void *scanner }

//标识tokens
%token  SEMICOLON
        CREATE
        DROP
        TABLE
        TABLES
        INDEX
        SELECT
        DESC
        SHOW
        SYNC
        INSERT
        DELETE
        UPDATE
        LBRACE
        RBRACE
        COMMA
        TRX_BEGIN
        TRX_COMMIT
        TRX_ROLLBACK
        INT_T
        STRING_T
        FLOAT_T
		DATE_T
		TEXT_T
        HELP
        EXIT
        DOT //QUOTE
        INTO
        VALUES
        FROM
        WHERE
        AND
        SET
        ON
        LOAD
        DATA
        INFILE
        EQ
        LT
        GT
        LE
        GE
        NE
		MAX
		MIN
		COUNT
		AVG
		SUM
		NOT
		LIKE
		INNER
		JOIN
		UNIQUE
		NULL_T
		NULLABLE_T
		IS_T

%union {
  struct _Attr *attr;
  struct _Condition *condition1;
  struct _Value *value1;
  char *string;
  int number;
  float floats;
  char *position;
}

%token <number> NUMBER
%token <floats> FLOAT 
%token <string> DATE_STR
%token <string> ID
%token <string> PATH
%token <string> SSS
%token <string> STAR
%token <string> STRING_V
//非终结符

%type <number> type;
%type <condition1> condition;
%type <value1> value;
%type <number> number;

%%

commands:		//commands or sqls. parser starts here.
    /* empty */
    | commands command
    ;

command:
	  select  
	| insert
	| update
	| delete
	| create_table
	| drop_table
	| show_tables
	| desc_table
	| create_index
	| show_index
	| drop_index
	| sync
	| begin
	| commit
	| rollback
	| load_data
	| help
	| exit
    ;

exit:			
    EXIT SEMICOLON {
        CONTEXT->ssql->flag=SCF_EXIT;//"exit";
    };

help:
    HELP SEMICOLON {
        CONTEXT->ssql->flag=SCF_HELP;//"help";
    };

sync:
    SYNC SEMICOLON {
      CONTEXT->ssql->flag = SCF_SYNC;
    }
    ;

begin:
    TRX_BEGIN SEMICOLON {
      CONTEXT->ssql->flag = SCF_BEGIN;
    }
    ;

commit:
    TRX_COMMIT SEMICOLON {
      CONTEXT->ssql->flag = SCF_COMMIT;
    }
    ;

rollback:
    TRX_ROLLBACK SEMICOLON {
      CONTEXT->ssql->flag = SCF_ROLLBACK;
    }
    ;

drop_table:		/*drop table 语句的语法解析树*/
    DROP TABLE ID SEMICOLON {
        CONTEXT->ssql->flag = SCF_DROP_TABLE;//"drop_table";
        drop_table_init(&CONTEXT->ssql->sstr.drop_table, $3);
    };

show_tables:
    SHOW TABLES SEMICOLON {
      CONTEXT->ssql->flag = SCF_SHOW_TABLES;
    }
    ;

desc_table:
    DESC ID SEMICOLON {
      CONTEXT->ssql->flag = SCF_DESC_TABLE;
      desc_table_init(&CONTEXT->ssql->sstr.desc_table, $2);
    }
    ;

create_index:		/*create index 语句的语法解析树*/
    CREATE INDEX ID ON ID LBRACE ID index_attr_list RBRACE SEMICOLON 
		{
			CONTEXT->ssql->flag = SCF_CREATE_INDEX;//"create_index";
			create_index_init(&CONTEXT->ssql->sstr.create_index, $3, $5);
			create_index_append(&CONTEXT->ssql->sstr.create_index,$7);
		}
	|CREATE UNIQUE INDEX ID ON ID LBRACE ID index_attr_list RBRACE SEMICOLON 
		{
			CONTEXT->ssql->sstr.create_index.unique = true;
			CONTEXT->ssql->flag = SCF_CREATE_INDEX;//"create_index";
			create_index_init(&CONTEXT->ssql->sstr.create_index, $4, $6);
			create_index_append(&CONTEXT->ssql->sstr.create_index,$8);
		}
    ;

index_attr_list:
	/* empty */
    | COMMA ID index_attr_list {
		create_index_append(&CONTEXT->ssql->sstr.create_index,$2);
	}
    ;

show_index:
	SHOW INDEX FROM ID SEMICOLON {
		CONTEXT->ssql->flag=SCF_SHOW_INDEX;//"show_index";
		show_index_init(&CONTEXT->ssql->sstr.show_index, $4);
	};

drop_index:			/*drop index 语句的语法解析树*/
    DROP INDEX ID  SEMICOLON 
		{
			CONTEXT->ssql->flag=SCF_DROP_INDEX;//"drop_index";
			drop_index_init(&CONTEXT->ssql->sstr.drop_index, $3);
		}
    ;
create_table:		/*create table 语句的语法解析树*/
    CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE SEMICOLON 
		{
			CONTEXT->ssql->flag=SCF_CREATE_TABLE;//"create_table";
			// CONTEXT->ssql->sstr.create_table.attribute_count = CONTEXT->value_length;
			create_table_init_name(&CONTEXT->ssql->sstr.create_table, $3);
			//临时变量清零	
			CONTEXT->value_length = 0;
		}
    ;
attr_def_list:
    /* empty */
    | COMMA attr_def attr_def_list {    }
    ;
    
attr_def:
    ID_get type LBRACE number RBRACE nullable
		{
			AttrInfo attribute;
			attr_info_init(&attribute, CONTEXT->id, $2, $4, CONTEXT->nullable);
			create_table_append_attribute(&CONTEXT->ssql->sstr.create_table, &attribute);
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name =(char*)malloc(sizeof(char));
			// strcpy(CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name, CONTEXT->id); 
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].type = $2;  
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].length = $4;
			CONTEXT->value_length++;
		}
	|ID_get type nullable
		{
			AttrInfo attribute;
			attr_info_init(&attribute, CONTEXT->id, $2, 4, CONTEXT->nullable);
			create_table_append_attribute(&CONTEXT->ssql->sstr.create_table, &attribute);
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name=(char*)malloc(sizeof(char));
			// strcpy(CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name, CONTEXT->id); 
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].type=$2;  
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].length=4; // default attribute length
			CONTEXT->value_length++;
		}
    ;
number:
		NUMBER {$$ = $1;}
		;
type:
	INT_T { $$=INTS; }
	| STRING_T { $$=CHARS; }
	| FLOAT_T { $$=FLOATS; }
	| DATE_T { $$=DATES; }
	| TEXT_T { $$=TEXTS; }
	;

ID_get:
	ID 
	{
		char *temp=$1; 
		snprintf(CONTEXT->id, sizeof(CONTEXT->id), "%s", temp);
	}
	;

nullable:
	/* empty */
	| NULLABLE_T {
		CONTEXT->nullable=1;
	}
	| NOT NULL_T {
		CONTEXT->nullable=0;
	}
	;

insert:				/*insert   语句的语法解析树*/
    INSERT INTO ID VALUES record record_list SEMICOLON 
		{
			CONTEXT->ssql->flag=SCF_INSERT;//"insert";
			inserts_init(&CONTEXT->ssql->sstr.insertion, $3, CONTEXT->record_length);
			//临时变量清零
			CONTEXT->value_length=0;
			CONTEXT->record_length=0;
    }

record_list:
	/* empty */
	| COMMA record record_list {

	};

record:
	| LBRACE value value_list RBRACE {
		inserts_record(&CONTEXT->ssql->sstr.insertion,CONTEXT->record_length++ ,CONTEXT->values, CONTEXT->value_length);
		CONTEXT->value_length=0;
	}
	;
value_list:
    /* empty */
    | COMMA value value_list  { 
  		// CONTEXT->values[CONTEXT->value_length++] = *$2;
	  }
    ;
value:
    NUMBER{	
  		value_init_integer(&CONTEXT->values[CONTEXT->value_length++], $1);
		}
    |FLOAT{
  		value_init_float(&CONTEXT->values[CONTEXT->value_length++], $1);
		}
	|DATE_STR{
		value_init_date(&CONTEXT->values[CONTEXT->value_length++], $1);
		}
    |SSS {
		$1 = substr($1,1,strlen($1)-2);
  		value_init_string(&CONTEXT->values[CONTEXT->value_length++], $1);
	}
	|NULL_T{
		value_init_null(&CONTEXT->values[CONTEXT->value_length++]);
	}
    ;
    
delete:		/*  delete 语句的语法解析树*/
    DELETE FROM ID where SEMICOLON 
		{
			CONTEXT->ssql->flag = SCF_DELETE;//"delete";
			deletes_init_relation(&CONTEXT->ssql->sstr.deletion, $3);
			deletes_set_conditions(&CONTEXT->ssql->sstr.deletion, 
					CONTEXT->conditions, CONTEXT->condition_length);
			CONTEXT->condition_length = 0;	
    }
    ;
update:			/*  update 语句的语法解析树*/
    UPDATE ID SET update_value update_value_list where SEMICOLON
		{
			CONTEXT->ssql->flag = SCF_UPDATE;//"update";
			
			updates_init(&CONTEXT->ssql->sstr.update, $2,CONTEXT->conditions, CONTEXT->condition_length);
			CONTEXT->condition_length = 0;
			CONTEXT->value_length = 0;
		}
    ;

update_value_list:
	 /* empty */
    | COMMA update_value update_value_list {
		
	};

update_value:
	/* empty */
    |ID EQ NUMBER{
		Value value;
  		value_init_integer(&value, $3);
		updates_value_append(&CONTEXT->ssql->sstr.update, $1, &value);
	}
    |ID EQ FLOAT{
		Value value;
  		value_init_float(&value, $3);
		updates_value_append(&CONTEXT->ssql->sstr.update, $1, &value);
	}
	|ID EQ DATE_STR{
		Value value;
  		value_init_date(&value, $3);
		updates_value_append(&CONTEXT->ssql->sstr.update, $1, &value);
	}
    |ID EQ SSS {
		$3 = substr($3,1,strlen($3)-2);
		Value value;
  		value_init_string(&value, $3);
		updates_value_append(&CONTEXT->ssql->sstr.update, $1, &value);
	}
	|ID EQ LBRACE SELECT select_attr FROM ID relation where RBRACE {
		// CONTEXT->selection.relations[CONTEXT->from_length++]=$4;
		selects_append_relation(&CONTEXT->selection, $7);

		selects_append_conditions(&CONTEXT->selection, CONTEXT->conditions, CONTEXT->condition_length);

		CONTEXT->ssql->flag=SCF_SELECT;//"select";
		// CONTEXT->selection.attr_num = CONTEXT->select_length;

		updates_select_append(&CONTEXT->ssql->sstr.update,$1, &CONTEXT->selection);

		//临时变量清零
		CONTEXT->condition_length=0;
		CONTEXT->from_length=0;
		CONTEXT->select_length=0;
		CONTEXT->value_length = 0;
	}
	|ID EQ LBRACE SELECT aggr_attr FROM ID rel_list where RBRACE {
		// CONTEXT->selection.relations[CONTEXT->from_length++]=$4;
		selects_append_relation(&CONTEXT->selection, $7);

		selects_append_conditions(&CONTEXT->selection, CONTEXT->conditions, CONTEXT->condition_length);

		CONTEXT->ssql->flag=SCF_SELECT;//"select";
		// CONTEXT->selection.attr_num = CONTEXT->select_length;

		updates_select_append(&CONTEXT->ssql->sstr.update,$1, &CONTEXT->selection);

		//临时变量清零
		CONTEXT->condition_length=0;
		CONTEXT->from_length=0;
		CONTEXT->select_length=0;
		CONTEXT->value_length = 0;
	}
    ;


select:				/*  select 语句的语法解析树*/
    SELECT select_attr FROM ID relation where SEMICOLON
		{
			// CONTEXT->selection.relations[CONTEXT->from_length++]=$4;
			selects_append_relation(&CONTEXT->selection, $4);

			selects_append_conditions(&CONTEXT->selection, CONTEXT->conditions, CONTEXT->condition_length);

			CONTEXT->ssql->flag=SCF_SELECT;//"select";
			// CONTEXT->selection.attr_num = CONTEXT->select_length;

			copy_selects(&CONTEXT->selection,&CONTEXT->ssql->sstr.selection);

			//临时变量清零
			CONTEXT->condition_length=0;
			CONTEXT->from_length=0;
			CONTEXT->select_length=0;
			CONTEXT->value_length = 0;
	}
	|SELECT aggr_attr FROM ID rel_list where SEMICOLON
		{
			// CONTEXT->selection.relations[CONTEXT->from_length++]=$4;
			selects_append_relation(&CONTEXT->selection, $4);

			selects_append_conditions(&CONTEXT->selection, CONTEXT->conditions, CONTEXT->condition_length);

			CONTEXT->ssql->flag=SCF_SELECT;//"select";
			// CONTEXT->selection.attr_num = CONTEXT->select_length;
			copy_selects(&CONTEXT->selection,&CONTEXT->ssql->sstr.selection);

			//临时变量清零
			CONTEXT->condition_length=0;
			CONTEXT->from_length=0;
			CONTEXT->select_length=0;
			CONTEXT->value_length = 0;
	}
	;

select_attr:
    STAR attr_list{  
			RelAttr attr;
			relation_attr_init(&attr, NULL, "*");
			selects_append_attribute(&CONTEXT->selection, &attr);
		}
    | ID attr_list {
			RelAttr attr;
			relation_attr_init(&attr, NULL, $1);
			selects_append_attribute(&CONTEXT->selection, &attr);
		}
  	| ID DOT ID attr_list {
			RelAttr attr;
			relation_attr_init(&attr, $1, $3);
			selects_append_attribute(&CONTEXT->selection, &attr);
		}
    ;
attr_list:
    /* empty */
    | COMMA ID attr_list {
			RelAttr attr;
			relation_attr_init(&attr, NULL, $2);
			selects_append_attribute(&CONTEXT->selection, &attr);
     	  // CONTEXT->selection.attributes[CONTEXT->select_length].relation_name = NULL;
        // CONTEXT->selection.attributes[CONTEXT->select_length++].attribute_name=$2;
      }
    | COMMA ID DOT ID attr_list {
			RelAttr attr;
			relation_attr_init(&attr, $2, $4);
			selects_append_attribute(&CONTEXT->selection, &attr);
        // CONTEXT->selection.attributes[CONTEXT->select_length].attribute_name=$4;
        // CONTEXT->selection.attributes[CONTEXT->select_length++].relation_name=$2;
  	  }
  	;

aggr_attr:
	aggr_value aggr_list {

	};

aggr_list:
	/* empty */
    | COMMA aggr_value aggr_list {	
				
	}
    ;

aggr_value:
	aggrOp LBRACE STAR RBRACE {
		AggrAttr aggr;
		aggr.aggr_type = CONTEXT->aggrOp;
		relation_attr_init(&aggr.rel_attr, NULL, "*");
		selects_append_aggregation(&CONTEXT->selection, &aggr);
	}
	|aggrOp LBRACE ID RBRACE {
		AggrAttr aggr;
		aggr.aggr_type = CONTEXT->aggrOp;
		relation_attr_init(&aggr.rel_attr, NULL, $3);
		selects_append_aggregation(&CONTEXT->selection, &aggr);
	}
	|aggrOp LBRACE ID DOT ID RBRACE {
		AggrAttr aggr;
		aggr.aggr_type = CONTEXT->aggrOp;
		relation_attr_init(&aggr.rel_attr, $3, $5);
		selects_append_aggregation(&CONTEXT->selection, &aggr);
	}
	|aggrOp LBRACE NUMBER RBRACE {
		AggrAttr aggr;
		aggr.aggr_type = CONTEXT->aggrOp;
		relation_attr_init(&aggr.rel_attr, NULL, "*");
		selects_append_aggregation(&CONTEXT->selection, &aggr);
	}

aggrOp:
	 MAX { CONTEXT->aggrOp = MAX_FUNC; }
    | MIN { CONTEXT->aggrOp = MIN_FUNC; }
    | COUNT{ CONTEXT->aggrOp = COUNT_FUNC; }
    | AVG { CONTEXT->aggrOp = AVG_FUNC; }
	| SUM { CONTEXT->aggrOp = SUM_FUNC; }
    ;

relation:
	/* empty */
	|COMMA ID rel_list {
		selects_append_relation(&CONTEXT->selection, $2);
	}
	|INNER JOIN ID ON condition join_list {
		selects_append_relation(&CONTEXT->selection, $3);
		selects_append_join_conditions(&CONTEXT->selection, CONTEXT->conditions, CONTEXT->condition_length);
		CONTEXT->condition_length = 0;
	}

rel_list:
    /* empty */
    | COMMA ID rel_list {	
				selects_append_relation(&CONTEXT->selection, $2);
		  }
    ;

join_list:
	/* empty */
	| INNER JOIN ID ON condition join_list{
		selects_append_relation(&CONTEXT->selection, $3);
	}
	| AND condition join_list {

	}
	;

where:
    /* empty */
    | WHERE condition condition_list {	
			// CONTEXT->conditions[CONTEXT->condition_length++]=*$2;
		}
    ;
condition_list:
    /* empty */
    | AND condition condition_list {
				// CONTEXT->conditions[CONTEXT->condition_length++]=*$2;
			}
    ;
condition:
    ID comOp value 
		{
			RelAttr left_attr;
			relation_attr_init(&left_attr, NULL, $1);

			Value *right_value = &CONTEXT->values[CONTEXT->value_length - 1];

			Condition condition;
			condition_init(&condition, CONTEXT->comp, 1, &left_attr, NULL, 0, NULL, right_value);
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;
			// $$ = ( Condition *)malloc(sizeof( Condition));
			// $$->left_is_attr = 1;
			// $$->left_attr.relation_name = NULL;
			// $$->left_attr.attribute_name= $1;
			// $$->comp = CONTEXT->comp;
			// $$->right_is_attr = 0;
			// $$->right_attr.relation_name = NULL;
			// $$->right_attr.attribute_name = NULL;
			// $$->right_value = *$3;

		}
		|value comOp value 
		{
			Value *left_value = &CONTEXT->values[CONTEXT->value_length - 2];
			Value *right_value = &CONTEXT->values[CONTEXT->value_length - 1];

			Condition condition;
			condition_init(&condition, CONTEXT->comp, 0, NULL, left_value, 0, NULL, right_value);
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;
			// $$ = ( Condition *)malloc(sizeof( Condition));
			// $$->left_is_attr = 0;
			// $$->left_attr.relation_name=NULL;
			// $$->left_attr.attribute_name=NULL;
			// $$->left_value = *$1;
			// $$->comp = CONTEXT->comp;
			// $$->right_is_attr = 0;
			// $$->right_attr.relation_name = NULL;
			// $$->right_attr.attribute_name = NULL;
			// $$->right_value = *$3;

		}
		|ID comOp ID 
		{
			RelAttr left_attr;
			relation_attr_init(&left_attr, NULL, $1);
			RelAttr right_attr;
			relation_attr_init(&right_attr, NULL, $3);

			Condition condition;
			condition_init(&condition, CONTEXT->comp, 1, &left_attr, NULL, 1, &right_attr, NULL);
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;
			// $$=( Condition *)malloc(sizeof( Condition));
			// $$->left_is_attr = 1;
			// $$->left_attr.relation_name=NULL;
			// $$->left_attr.attribute_name=$1;
			// $$->comp = CONTEXT->comp;
			// $$->right_is_attr = 1;
			// $$->right_attr.relation_name=NULL;
			// $$->right_attr.attribute_name=$3;

		}
    |value comOp ID
		{
			Value *left_value = &CONTEXT->values[CONTEXT->value_length - 1];
			RelAttr right_attr;
			relation_attr_init(&right_attr, NULL, $3);

			Condition condition;
			condition_init(&condition, CONTEXT->comp, 0, NULL, left_value, 1, &right_attr, NULL);
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;

			// $$=( Condition *)malloc(sizeof( Condition));
			// $$->left_is_attr = 0;
			// $$->left_attr.relation_name=NULL;
			// $$->left_attr.attribute_name=NULL;
			// $$->left_value = *$1;
			// $$->comp=CONTEXT->comp;
			
			// $$->right_is_attr = 1;
			// $$->right_attr.relation_name=NULL;
			// $$->right_attr.attribute_name=$3;
		
		}
    |ID DOT ID comOp value
		{
			RelAttr left_attr;
			relation_attr_init(&left_attr, $1, $3);
			Value *right_value = &CONTEXT->values[CONTEXT->value_length - 1];

			Condition condition;
			condition_init(&condition, CONTEXT->comp, 1, &left_attr, NULL, 0, NULL, right_value);
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;

			// $$=( Condition *)malloc(sizeof( Condition));
			// $$->left_is_attr = 1;
			// $$->left_attr.relation_name=$1;
			// $$->left_attr.attribute_name=$3;
			// $$->comp=CONTEXT->comp;
			// $$->right_is_attr = 0;   //属性值
			// $$->right_attr.relation_name=NULL;
			// $$->right_attr.attribute_name=NULL;
			// $$->right_value =*$5;			
							
    }
    |value comOp ID DOT ID
		{
			Value *left_value = &CONTEXT->values[CONTEXT->value_length - 1];

			RelAttr right_attr;
			relation_attr_init(&right_attr, $3, $5);

			Condition condition;
			condition_init(&condition, CONTEXT->comp, 0, NULL, left_value, 1, &right_attr, NULL);
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;
			// $$=( Condition *)malloc(sizeof( Condition));
			// $$->left_is_attr = 0;//属性值
			// $$->left_attr.relation_name=NULL;
			// $$->left_attr.attribute_name=NULL;
			// $$->left_value = *$1;
			// $$->comp =CONTEXT->comp;
			// $$->right_is_attr = 1;//属性
			// $$->right_attr.relation_name = $3;
			// $$->right_attr.attribute_name = $5;
									
    }
    |ID DOT ID comOp ID DOT ID
		{
			RelAttr left_attr;
			relation_attr_init(&left_attr, $1, $3);
			RelAttr right_attr;
			relation_attr_init(&right_attr, $5, $7);

			Condition condition;
			condition_init(&condition, CONTEXT->comp, 1, &left_attr, NULL, 1, &right_attr, NULL);
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;
			// $$=( Condition *)malloc(sizeof( Condition));
			// $$->left_is_attr = 1;		//属性
			// $$->left_attr.relation_name=$1;
			// $$->left_attr.attribute_name=$3;
			// $$->comp =CONTEXT->comp;
			// $$->right_is_attr = 1;		//属性
			// $$->right_attr.relation_name=$5;
			// $$->right_attr.attribute_name=$7;
    }
    ;

comOp:
  	  EQ { CONTEXT->comp = EQUAL_TO; }
    | LT { CONTEXT->comp = LESS_THAN; }
    | GT { CONTEXT->comp = GREAT_THAN; }
    | LE { CONTEXT->comp = LESS_EQUAL; }
    | GE { CONTEXT->comp = GREAT_EQUAL; }
    | NE { CONTEXT->comp = NOT_EQUAL; }
	| LIKE { CONTEXT->comp = LIKE_TO; }
	| NOT LIKE { CONTEXT->comp = NOT_LIKE; }
	| IS_T { CONTEXT->comp = IS_TO; }
	| IS_T NOT { CONTEXT->comp = IS_NOT; }
    ;

load_data:
		LOAD DATA INFILE SSS INTO TABLE ID SEMICOLON
		{
		  CONTEXT->ssql->flag = SCF_LOAD_DATA;
			load_data_init(&CONTEXT->ssql->sstr.load_data, $7, $4);
		}
		;
%%
//_____________________________________________________________________
extern void scan_string(const char *str, yyscan_t scanner);

int sql_parse(const char *s, Query *sqls){
	ParserContext context;
	memset(&context, 0, sizeof(context));

	yyscan_t scanner;
	yylex_init_extra(&context, &scanner);
	context.ssql = sqls;
	scan_string(s, scanner);
	int result = yyparse(scanner);
	yylex_destroy(scanner);
	return result;
}
