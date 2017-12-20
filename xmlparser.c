#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "dbg.h"
#include "array.h"

#define CHECK_NULL(p, M, ...) if(p==NULL){debug(M, ##__VA_ARGS__); return 0; };
#define JUMP_IF(cond, M, ...) if(cond) {debug(M, ##__VA_ARGS__); goto error; };

/*
	越过注释 <!--...-->
*/
#define SKIP_COMMENT(p) \
	if(*p=='<' && *(p+1)== '!' && *(p+2)=='-' && *(p+3)=='-') \
	{\
		p+=4;\
		while(1) \
		{\
			if(*p == 0) goto error;\
			if(*p=='-' && *(p+1) == '-' && *(p+2) == '>') \
			{\
				p+=3;\
				break;\
			}\
			p++;\
		}\
	}

/*
	越过空格，包括空格和制表符
*/
#define SKIP_SPACE(p) \
	while(1) \
	{\
		if(*p == ' ' || *p == '\t') p++; \
		else if(*p == 0 || *p=='\r' || *p == '\n') \
			goto error;\
		else \
			break;\
	}

/*
	越过空白字符
*/
#define SKIP_BLANK(p) \
	while(1) \
	{\
		if(*p == ' ' || *p == '\t' || *p=='\r' || *p=='\n') p++; \
		else if(*p == 0) \
			goto error;\
		else \
			break;\
	}

/*
	同上，只不过遇到\0不出错
*/
#define SKIP_BLANK_DONE(p) \
	while(1) \
	{\
		if(*p == ' ' || *p == '\t' || *p=='\r' || *p=='\n') p++; \
		else \
			break;\
	}


typedef struct xml_attri{
	char* name;
	char* value;
}XML_ATTRI;

typedef struct xml_node{
	ARRAY attris;

	ARRAY children;

	struct xml_node * parent;

	char* name;

	//暂时只支持一个text
	char* text;
	
}XML_NODE;

/* 解析节点 */
int parser_node_start(XML_NODE* parent, char* p, char** ppEnd);

/* 解析属性 */
int parser_attri_start(XML_NODE* node, char* p, char** ppEnd);

/* 解析text */
int parser_text_start(XML_NODE* node, char* p, char** ppEnd);

static XML_NODE * s_root_node = NULL;

void free_xml_node(XML_NODE* node)
{
	size_t i;

	for(i=0;i<node->attris.num;i++)
	{
		XML_ATTRI * attri = (XML_ATTRI*) node->attris.item[i];
		if(attri->name) free(attri->name);
		if(attri->value) free(attri->value);
	}
	if(node->attris.item) free(node->attris.item);

	for(i=0;i<node->children.num;i++)
	{
		XML_NODE * child = (XML_NODE*) node->children.item[i];
		free_xml_node(child);
	}
	if(node->children.item) free(node->children.item);

	if(node->name) free(node->name);
	if(node->text) free(node->text);

	memset(node, 0 ,sizeof(XML_NODE));

	free(node);
}

char* get_name_by_spec(char* p, char c, char** ppEnd)
{
	char* e, *name;

	e = p;

	while(1)
	{
		JUMP_IF(*e == 0 || *e == '\r' || *e == '\n', "parser name error");

		if(*e == ' ' || *e == '\t')
			break;
		if(c != 0 && *e==c)
			break;
		e++;
	}
	if(e==p) return NULL;

	name = (char*)malloc(e - p +1);
	JUMP_IF(name == NULL, "malloc return NULL");

	strncpy(name, p, e-p);
	name[e-p] = 0;

	*ppEnd = e;

	return name;

error:
	return NULL;
}

int parser_node_start(XML_NODE* parent, char* p, char** ppEnd)
{
	char* s, *e, *name;
	XML_NODE * node = NULL;
	int ret, i, len;

	s = p;
	SKIP_BLANK(s);

	SKIP_COMMENT(s);

	SKIP_BLANK(s);

	JUMP_IF(*s != '<', "can't find node start");
	s++;

	SKIP_SPACE(s);
	name = get_name_by_spec(s, '>', ppEnd);
	JUMP_IF(name==NULL, "get name return NULL");

	s = *ppEnd;

	node = (XML_NODE*)malloc(sizeof(XML_NODE));
	JUMP_IF(node == NULL, "malloc return NULL");

	memset(node, 0, sizeof(XML_NODE));

	node->name = name;

	SKIP_SPACE(s);

	if(*s != '>')
	{
		ret = parser_attri_start(node, s, ppEnd);
		JUMP_IF(ret == 0, "parser attribute error");

		s = *ppEnd;
		
	}

	ret = parser_text_start(node, s+1, ppEnd);
	JUMP_IF(ret == 0, "parser text error");


	while(1)
	{
		s = *ppEnd;
		SKIP_BLANK(s);

		JUMP_IF(*s!='<', "parser tag %s",name);
		e = s;
		s++;

		SKIP_SPACE(s);
		if(*s=='/')
		{
			s++;
			goto node_end;
		}
		else
		{
			s = e;
			ret = parser_node_start(node, s, ppEnd);
				JUMP_IF(ret == 0, "parser %s's children error",name);
		}

	}

node_end:

	SKIP_SPACE(s);
	if(strncmp(s, name, strlen(name)) != 0)
	{
		debug("parser tag %s",name);
		goto error;
	}
	s+=strlen(name);

	JUMP_IF(*s!='>', "parser tag %s",name);
	s++;

	SKIP_BLANK_DONE(s);
	*ppEnd = s;

	node->parent = parent;
	if(parent)
	{
		if(!expand_array(&parent->children, 1)) goto error;
		parent->children.item[parent->children.num++] = node;
	}
	else
	{
		s_root_node = node;	
	}

	return 1;
error:
	if(node) free_xml_node(node);
	return 0;
}

int parser_attri_start(XML_NODE* node, char* p, char** ppEnd)
{
	char* s, *e, *name = NULL, *value = NULL;
	int ret;
	XML_ATTRI * xmlAttri = NULL;

	s = p;
	SKIP_BLANK(s);

	while(1)
	{
		name = get_name_by_spec(s,'=',ppEnd);
		JUMP_IF(name==NULL,"attri get name error");

		s = *ppEnd;
		SKIP_SPACE(s);

		JUMP_IF(*s!='=', "attri error");
		s++;
		SKIP_SPACE(s);
		JUMP_IF(*s != '\"', "attri error");
		s++;
		e = s;
		while(*e != '\"') e++;

		if(e==s) value = NULL;
		else
		{
			value = (char*)malloc(e-s+1);
			JUMP_IF(value==NULL, "malloc return NULL");

			strncpy(value, s, e-s);
			value[e-s] = 0;
		}

		XML_ATTRI * xmlAttri = (XML_ATTRI*)malloc(sizeof(XML_ATTRI));
		JUMP_IF(xmlAttri==NULL, "malloc return NULL");

		xmlAttri->name = name;
		xmlAttri->value = value;

		JUMP_IF((expand_array(&node->attris, 1) != 1), "expand array for attribute error");
		node->attris.item[node->attris.num++] = xmlAttri;
		
		s = e+1;
		SKIP_BLANK(s);

		if(*s == '>')
		{
			*ppEnd = s;
			break;
		}

	}
	
	return 1;
error:

	return 0;	
}

int parser_text_start(XML_NODE* node, char* p, char** ppEnd)
{
	char *s, *e, *text;
	int n = 0;

	s = p;
	SKIP_BLANK(s);

	if(*s == '<')
	{
		*ppEnd = s;
		return 1;
	}

	e = s;
	while(1)
	{
		if(*e == '<' || *e==' ' || *e== '\t' || *e=='\r' || *e =='\n')
		{
			text = malloc(e-s+n+2);
			JUMP_IF(text==NULL, "malloc return NULL");

			text[0]=0;

			if(node->text)
			{
				strcpy(text, node->text);
				free(node->text);
				text[n] = ' ';
				n++;
			}
	
			strncpy(text+n, s, e-s);
			n+=e-s;
			text[n] = 0;

			node->text = text;

			if(*e == '<')
			{
				s = e;
				break;
			}
			else
			{
				SKIP_BLANK(e);
				s = e;
			}
		}
		else
		{
			e++;
		}

	}

	*ppEnd = s;
	return 1;
error:
	return 0;
}

void show_node(int tabs, XML_NODE * node)
{
	size_t i;
	char padding[50] = {0};

	for(i=0;i<tabs;i++)
	{
		padding[i] = padding[i + tabs] = ' ';
	}

	printf("%s<%s",padding, node->name);

	for(i=0;i<node->attris.num;i++)
	{
		XML_ATTRI * attri = (XML_ATTRI*) node->attris.item[i];
		if(attri->name)
		{
			printf(" %s=",attri->name);
		}
		if(attri->value)
		{
			printf("%s",attri->value);
		}
	}

	printf(">\n");

	if(node->text)
	{
		printf("%s%s\n",padding, node->text);
	}

	for(i=0;i<node->children.num;i++)
	{
		XML_NODE * child = (XML_NODE*) node->children.item[i];
		show_node(tabs+1, child);
	}

	printf("%s</%s>\n",padding,node->name);

}

static char * readFileGetBuff(char * name, int * pFileLen)
{
	FILE * fp;
	int n;
	char * buff;
	int result;
	int fileLen;

	fp = fopen(name,"rb");
	if(fp == NULL)
	{
		printf("fopen failed\n");
		return NULL;
	}
	result = fseek(fp,0,SEEK_END);
	if(result)
	{
		printf("fseek failed\n");
		fclose(fp);
		return NULL;
	}
	fileLen = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	buff = (char *)malloc(fileLen+1);
	n = fread(buff, 1, fileLen, fp);
	buff[n] = 0;
	fclose(fp);
	*pFileLen = n;
	return buff;
}

int main()
{
	char* file = "test.xml", *buf;
	char* pEnd;
	int filelen, ret;
	
	buf = readFileGetBuff(file, &filelen);
	if(buf == NULL)
	{
		printf("file is empty\n");
		return 0;
	}

	ret = parser_node_start(NULL, buf, &pEnd);
	if(ret == 1)
		show_node(0, s_root_node);
	return 0;
}
