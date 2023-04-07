#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "blank.h"

// save data types
char datatype[DATATYPE_SIZE][MINLEN] = {"int", "char", "double", "float", "long"
			, "short", "ushort", "FILE", "DIR","pid"
			,"key_t", "ssize_t", "mode_t", "ino_t", "dev_t"
			, "nlink_t", "uid_t", "gid_t", "time_t", "blksize_t"
			, "blkcnt_t", "pid_t", "pthread_mutex_t", "pthread_cond_t", "pthread_t"
			, "void", "size_t", "unsigned", "sigset_t", "sigjmp_buf"
			, "rlim_t", "jmp_buf", "sig_atomic_t", "clock_t", "struct"};


// operator rank
operator_precedence operators[OPERATOR_CNT] = {
	{"(", 0}, {")", 0}
	,{"->", 1}	
	,{"*", 4}	,{"/", 3}	,{"%", 2}	
	,{"+", 6}	,{"-", 5}	
	,{"<", 7}	,{"<=", 7}	,{">", 7}	,{">=", 7}
	,{"==", 8}	,{"!=", 8}
	,{"&", 9}
	,{"^", 10}
	,{"|", 11}
	,{"&&", 12}
	,{"||", 13}
	,{"=", 14}	,{"+=", 14}	,{"-=", 14}	,{"&=", 14}	,{"|=", 14}
};

void compare_tree(node *root1,  node *root2, int *result)
{
	node *tmp;
	int cnt1, cnt2;

	if(root1 == NULL || root2 == NULL){
		*result = false;
		return;
	}

	/* when root1's name is < or > or <= or >= */
	if(!strcmp(root1->name, "<") || !strcmp(root1->name, ">") || !strcmp(root1->name, "<=") || !strcmp(root1->name, ">=")){
		// when root1's name and root2's name are different
		if(strcmp(root1->name, root2->name) != 0){
			// if first char is < then change to >
			if(!strncmp(root2->name, "<", 1))
				strncpy(root2->name, ">", 1);

			// if first char is > then change to >
			else if(!strncmp(root2->name, ">", 1))
				strncpy(root2->name, "<", 1);

			// if first char is <= then change to >=
			else if(!strncmp(root2->name, "<=", 2))
				strncpy(root2->name, ">=", 2);

			// if first char is >= then change to <=
			else if(!strncmp(root2->name, ">=", 2))
				strncpy(root2->name, "<=", 2);

			root2 = change_sibling(root2);
		}
	}

	if(strcmp(root1->name, root2->name) != 0){
		*result = false;
		return;
	}

	if((root1->child_head != NULL && root2->child_head == NULL)
		|| (root1->child_head == NULL && root2->child_head != NULL)){
		*result = false;
		return;
	}

	else if(root1->child_head != NULL){
		if(get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)){
			*result = false;
			return;
		}

		if(!strcmp(root1->name, "==") || !strcmp(root1->name, "!="))
		{
			compare_tree(root1->child_head, root2->child_head, result);

			if(*result == false)
			{
				*result = true;
				root2 = change_sibling(root2);
				compare_tree(root1->child_head, root2->child_head, result);
			}
		}
		else if(!strcmp(root1->name, "+") || !strcmp(root1->name, "*")
				|| !strcmp(root1->name, "|") || !strcmp(root1->name, "&")
				|| !strcmp(root1->name, "||") || !strcmp(root1->name, "&&"))
		{
			if(get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)){
				*result = false;
				return;
			}

			tmp = root2->child_head;

			while(tmp->prev != NULL)
				tmp = tmp->prev;

			while(tmp != NULL)
			{
				compare_tree(root1->child_head, tmp, result);
			
				if(*result == true)
					break;
				else{
					if(tmp->next != NULL)
						*result = true;
					tmp = tmp->next;
				}
			}
		}
		else{
			compare_tree(root1->child_head, root2->child_head, result);
		}
	}	


	if(root1->next != NULL){

		if(get_sibling_cnt(root1) != get_sibling_cnt(root2)){
			*result = false;
			return;
		}

		if(*result == true)
		{
			tmp = get_operator(root1);
	
			if(!strcmp(tmp->name, "+") || !strcmp(tmp->name, "*")
					|| !strcmp(tmp->name, "|") || !strcmp(tmp->name, "&")
					|| !strcmp(tmp->name, "||") || !strcmp(tmp->name, "&&"))
			{	
				tmp = root2;
	
				while(tmp->prev != NULL)
					tmp = tmp->prev;

				while(tmp != NULL)
				{
					compare_tree(root1->next, tmp, result);

					if(*result == true)
						break;
					else{
						if(tmp->next != NULL)
							*result = true;
						tmp = tmp->next;
					}
				}
			}

			else
				compare_tree(root1->next, root2->next, result);
		}
	}
}

/* called in score_blank */
/* tokenize student's answer and save to tokens */
/* str : student's answer, tokens : dest to save */
int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN])
{
	char *start, *end;
	char tmp[BUFLEN];
	char str2[BUFLEN];
	char *op = "(),;><=!|&^/+-*\"";  // operations
	int row = 0;
	int i;
 	int isPointer;
	int lcount, rcount;  // (, ) nums
	int p_str;
	
	clear_tokens(tokens);  // init tokens 0

	start = str;  // start points str
	
	if(is_typeStatement(str) == 0)  // check str is wrong answer
		return false;	
	
	while(1)
	{
		if((end = strpbrk(start, op)) == NULL)  // if s_answer has no op
			break;

		if(start == end){  // s_ans[0] starts with op

			if(!strncmp(start, "--", 2) || !strncmp(start, "++", 2)){  // s_ans starts with -- or ++
				if(!strncmp(start, "++++", 4)||!strncmp(start,"----",4))  // s_ans starts with ++++ or ----
					return false;  // wrong answer

				// ex) ++a
				if(is_character(*ltrim(start + 2))){  // if char behind ++ [0-9a-zA-Z]
					if(row > 0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]))
						return false; //ex) ++a++

					end = strpbrk(start + 2, op);  // find next op after ++
					if(end == NULL)  // there's no op excepts for front ++
						end = &str[strlen(str)];  // end points end of s_ans
					// save tokens from s_ans
					while(start < end) {
						if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))  // prev char is space && current token's last char is [0-9a-Z]
							return false;
						else if(*start != ' ')  // save current char
							strncat(tokens[row], start, 1);  // concat current char to row'th token
						start++;  // points next char
					}
				}
				// ex) a++
				else if(row>0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){  // 
					if(strstr(tokens[row - 1], "++") != NULL || strstr(tokens[row - 1], "--") != NULL)	
						return false;

					memset(tmp, 0, sizeof(tmp));
					strncpy(tmp, start, 2);
					strcat(tokens[row - 1], tmp);
					start += 2;
					row--;
				}
				else{
					memset(tmp, 0, sizeof(tmp));
					strncpy(tmp, start, 2);
					strcat(tokens[row], tmp);
					start += 2;
				}
			}

			else if(!strncmp(start, "==", 2) || !strncmp(start, "!=", 2) || !strncmp(start, "<=", 2)
				|| !strncmp(start, ">=", 2) || !strncmp(start, "||", 2) || !strncmp(start, "&&", 2) 
				|| !strncmp(start, "&=", 2) || !strncmp(start, "^=", 2) || !strncmp(start, "!=", 2) 
				|| !strncmp(start, "|=", 2) || !strncmp(start, "+=", 2)	|| !strncmp(start, "-=", 2) 
				|| !strncmp(start, "*=", 2) || !strncmp(start, "/=", 2)){

				strncpy(tokens[row], start, 2);
				start += 2;
			}
			else if(!strncmp(start, "->", 2))
			{
				end = strpbrk(start + 2, op);

				if(end == NULL)
					end = &str[strlen(str)];

				while(start < end){
					if(*start != ' ')
						strncat(tokens[row - 1], start, 1);
					start++;
				}
				row--;
			}
			else if(*end == '&')
			{
				// ex) &a (address)
				if(row == 0 || (strpbrk(tokens[row - 1], op) != NULL)){
					end = strpbrk(start + 1, op);
					if(end == NULL)
						end = &str[strlen(str)];
					
					strncat(tokens[row], start, 1);
					start++;

					while(start < end){
						if(*(start - 1) == ' ' && tokens[row][strlen(tokens[row]) - 1] != '&')
							return false;
						else if(*start != ' ')
							strncat(tokens[row], start, 1);
						start++;
					}
				}
				// ex) a & b (bit)
				else{
					strncpy(tokens[row], start, 1);
					start += 1;
				}
				
			}
		  	else if(*end == '*')
			{
				isPointer=0;

				if(row > 0)
				{
					//ex) char** (pointer)
					for(i = 0; i < DATATYPE_SIZE; i++) {
						if(strstr(tokens[row - 1], datatype[i]) != NULL){
							strcat(tokens[row - 1], "*");
							start += 1;	
							isPointer = 1;
							break;
						}
					}
					if(isPointer == 1)
						continue;
					if(*(start+1) !=0)
						end = start + 1;

					// ex) a * **b (multiply then pointer)
					if(row>1 && !strcmp(tokens[row - 2], "*") && (all_star(tokens[row - 1]) == 1)){
						strncat(tokens[row - 1], start, end - start);
						row--;
					}
					
					// ex) a*b(multiply)
					else if(is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]) == 1){ 
						strncat(tokens[row], start, end - start);   
					}

					// ex) ,*b (pointer)
					else if(strpbrk(tokens[row - 1], op) != NULL){		
						strncat(tokens[row] , start, end - start); 
							
					}
					else
						strncat(tokens[row], start, end - start);

					start += (end - start);
				}

			 	else if(row == 0)
				{
					if((end = strpbrk(start + 1, op)) == NULL){
						strncat(tokens[row], start, 1);
						start += 1;
					}
					else{
						while(start < end){
							if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))
								return false;
							else if(*start != ' ')
								strncat(tokens[row], start, 1);
							start++;	
						}
						if(all_star(tokens[row]))
							row--;
						
					}
				}
			}
			else if(*end == '(') 
			{
				lcount = 0;
				rcount = 0;
				if(row>0 && (strcmp(tokens[row - 1],"&") == 0 || strcmp(tokens[row - 1], "*") == 0)){
					while(*(end + lcount + 1) == '(')
						lcount++;
					start += lcount;

					end = strpbrk(start + 1, ")");

					if(end == NULL)
						return false;
					else{
						while(*(end + rcount +1) == ')')
							rcount++;
						end += rcount;

						if(lcount != rcount)
							return false;

						if( (row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1])) || row == 1){ 
							strncat(tokens[row - 1], start + 1, end - start - rcount - 1);
							row--;
							start = end + 1;
						}
						else{
							strncat(tokens[row], start, 1);
							start += 1;
						}
					}
						
				}
				else{
					strncat(tokens[row], start, 1);
					start += 1;
				}

			}
			else if(*end == '\"') 
			{
				end = strpbrk(start + 1, "\"");
				
				if(end == NULL)
					return false;

				else{
					strncat(tokens[row], start, end - start + 1);
					start = end + 1;
				}

			}

			else{
				// ex) a++ ++ +b
				if(row > 0 && !strcmp(tokens[row - 1], "++"))
					return false;

				// ex) a-- -- -b
				if(row > 0 && !strcmp(tokens[row - 1], "--"))
					return false;
	
				strncat(tokens[row], start, 1);
				start += 1;
				
				// ex) -a or a, -b
				if(!strcmp(tokens[row], "-") || !strcmp(tokens[row], "+") || !strcmp(tokens[row], "--") || !strcmp(tokens[row], "++")){


					// ex) -a or -a+b
					if(row == 0)
						row--;

					// ex) a+b = -c
					else if(!is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){
					
						if(strstr(tokens[row - 1], "++") == NULL && strstr(tokens[row - 1], "--") == NULL)
							row--;
					}
				}
			}
		}
		else{  // s_ans doesn't start with op
			if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))  // prev token is 1 or more *s && prev prev token's last char is not character
				row--;				

			if(all_star(tokens[row - 1]) && row == 1)  // prev token is 1 or more *s && current row = 1
				row--;	

			// save keyword to current token
			for(i = 0; i < end - start; i++){  // end - start is keyword in front of op. ex) lseek, creat
				if(i > 0 && *(start + i) == '.'){  // if keyword has .
					strncat(tokens[row], start + i, 1);  // add . to current token

					while( *(start + i +1) == ' ' && i< end - start )  // space skip
						i++; 
				}
				else if(start[i] == ' '){  // if start starts with space
					while(start[i] == ' ')  // skip spaces
						i++;
					break;
				}
				else
					strncat(tokens[row], start + i, 1);  // add keyword's char to current token
			}

			if(start[0] == ' '){  // if start[0] is space
				start += i;
				continue;
			}
			start += i;
		}
			
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));  // remove spaces in token

		 if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1]) 
				&& (is_typeStatement(tokens[row - 1]) == 2 
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.' ) ){

			if(row > 1 && strcmp(tokens[row - 2],"(") == 0)
			{
				if(strcmp(tokens[row - 1], "struct") != 0 && strcmp(tokens[row - 1],"unsigned") != 0)
					return false;
			}
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)	
					return false;
			}
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
			
		}

		if((row == 0 && !strcmp(tokens[row], "gcc")) ){
			clear_tokens(tokens);
			strcpy(tokens[0], str);	
			return 1;
		} 

		row++;
	}

	if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))  
		row--;				
	if(all_star(tokens[row - 1]) && row == 1)   
		row--;	

	for(i = 0; i < strlen(start); i++)   
	{
		if(start[i] == ' ')  
		{
			while(start[i] == ' ')
				i++;
			if(start[0]==' ') {
				start += i;
				i = 0;
			}
			else
				row++;
			
			i--;
		} 
		else
		{
			strncat(tokens[row], start + i, 1);
			if( start[i] == '.' && i<strlen(start)){
				while(start[i + 1] == ' ' && i < strlen(start))
					i++;

			}
		}
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));

		if(!strcmp(tokens[row], "lpthread") && row > 0 && !strcmp(tokens[row - 1], "-")){ 
			strcat(tokens[row - 1], tokens[row]);
			memset(tokens[row], 0, sizeof(tokens[row]));
			row--;
		}
	 	else if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1]) 
				&& (is_typeStatement(tokens[row - 1]) == 2 
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.') ){
			
			if(row > 1 && strcmp(tokens[row-2],"(") == 0)
			{
				if(strcmp(tokens[row-1], "struct") != 0 && strcmp(tokens[row-1], "unsigned") != 0)
					return false;
			}
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)	
					return false;
			}
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
		} 
	}


	if(row > 0)
	{

		// ex) #include <sys/types.h>
		if(strcmp(tokens[0], "#include") == 0 || strcmp(tokens[0], "include") == 0 || strcmp(tokens[0], "struct") == 0){ 
			clear_tokens(tokens); 
			strcpy(tokens[0], remove_extraspace(str)); 
		}
	}

	if(is_typeStatement(tokens[0]) == 2 || strstr(tokens[0], "extern") != NULL){
		for(i = 1; i < TOKEN_CNT; i++){
			if(strcmp(tokens[i],"") == 0)  
				break;		       

			if(i != TOKEN_CNT -1 )
				strcat(tokens[0], " ");
			strcat(tokens[0], tokens[i]);
			memset(tokens[i], 0, sizeof(tokens[i]));
		}
	}
	
	//change ( ' char ' )' a  ->  (char)a
	while((p_str = find_typeSpecifier(tokens)) != -1){ 
		if(!reset_tokens(p_str, tokens))
			return false;
	}

	//change sizeof ' ( ' record ' ) '-> sizeof(record)
	while((p_str = find_typeSpecifier2(tokens)) != -1){  
		if(!reset_tokens(p_str, tokens))
			return false;
	}
	
	return true;
}

/* called in score_blank */
/* make lcrs tree with s_ans's tokens*/
node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses)
{
	node *cur = root;
	node *new;
	node *saved_operator;
	node *operator;
	int fstart;
	int i;

	while(1)	
	{
		if(strcmp(tokens[*idx], "") == 0)
			break;
	
		if(!strcmp(tokens[*idx], ")"))
			return get_root(cur);

		else if(!strcmp(tokens[*idx], ","))
			return get_root(cur);

		else if(!strcmp(tokens[*idx], "("))
		{
			// function()
			if(*idx > 0 && !is_operator(tokens[*idx - 1]) && strcmp(tokens[*idx - 1], ",") != 0){
				fstart = true;

				while(1)
				{
					*idx += 1;

					if(!strcmp(tokens[*idx], ")"))
						break;
					
					new = make_tree(NULL, tokens, idx, parentheses + 1);
					
					if(new != NULL){
						if(fstart == true){
							cur->child_head = new;
							new->parent = cur;
	
							fstart = false;
						}
						else{
							cur->next = new;
							new->prev = cur;
						}

						cur = new;
					}

					if(!strcmp(tokens[*idx], ")"))
						break;
				}
			}
			else{
				*idx += 1;
	
				new = make_tree(NULL, tokens, idx, parentheses + 1);

				if(cur == NULL)
					cur = new;

				else if(!strcmp(new->name, cur->name)){
					if(!strcmp(new->name, "|") || !strcmp(new->name, "||") 
						|| !strcmp(new->name, "&") || !strcmp(new->name, "&&"))
					{
						cur = get_last_child(cur);

						if(new->child_head != NULL){
							new = new->child_head;

							new->parent->child_head = NULL;
							new->parent = NULL;
							new->prev = cur;
							cur->next = new;
						}
					}
					else if(!strcmp(new->name, "+") || !strcmp(new->name, "*"))
					{
						i = 0;

						while(1)
						{
							if(!strcmp(tokens[*idx + i], ""))
								break;

							if(is_operator(tokens[*idx + i]) && strcmp(tokens[*idx + i], ")") != 0)
								break;

							i++;
						}
						
						if(get_precedence(tokens[*idx + i]) < get_precedence(new->name))
						{
							cur = get_last_child(cur);
							cur->next = new;
							new->prev = cur;
							cur = new;
						}
						else
						{
							cur = get_last_child(cur);

							if(new->child_head != NULL){
								new = new->child_head;

								new->parent->child_head = NULL;
								new->parent = NULL;
								new->prev = cur;
								cur->next = new;
							}
						}
					}
					else{
						cur = get_last_child(cur);
						cur->next = new;
						new->prev = cur;
						cur = new;
					}
				}
	
				else
				{
					cur = get_last_child(cur);

					cur->next = new;
					new->prev = cur;
	
					cur = new;
				}
			}
		}
		else if(is_operator(tokens[*idx]))
		{
			if(!strcmp(tokens[*idx], "||") || !strcmp(tokens[*idx], "&&")
					|| !strcmp(tokens[*idx], "|") || !strcmp(tokens[*idx], "&") 
					|| !strcmp(tokens[*idx], "+") || !strcmp(tokens[*idx], "*"))
			{
				if(is_operator(cur->name) == true && !strcmp(cur->name, tokens[*idx]))
					operator = cur;
		
				else
				{
					new = create_node(tokens[*idx], parentheses);
					operator = get_most_high_precedence_node(cur, new);

					if(operator->parent == NULL && operator->prev == NULL){

						if(get_precedence(operator->name) < get_precedence(new->name)){
							cur = insert_node(operator, new);
						}

						else if(get_precedence(operator->name) > get_precedence(new->name))
						{
							if(operator->child_head != NULL){
								operator = get_last_child(operator);
								cur = insert_node(operator, new);
							}
						}
						else
						{
							operator = cur;
	
							while(1)
							{
								if(is_operator(operator->name) == true && !strcmp(operator->name, tokens[*idx]))
									break;
						
								if(operator->prev != NULL)
									operator = operator->prev;
								else
									break;
							}

							if(strcmp(operator->name, tokens[*idx]) != 0)
								operator = operator->parent;

							if(operator != NULL){
								if(!strcmp(operator->name, tokens[*idx]))
									cur = operator;
							}
						}
					}

					else
						cur = insert_node(operator, new);
				}

			}
			else
			{
				new = create_node(tokens[*idx], parentheses);

				if(cur == NULL)
					cur = new;

				else
				{
					operator = get_most_high_precedence_node(cur, new);

					if(operator->parentheses > new->parentheses)
						cur = insert_node(operator, new);

					else if(operator->parent == NULL && operator->prev ==  NULL){
					
						if(get_precedence(operator->name) > get_precedence(new->name))
						{
							if(operator->child_head != NULL){
	
								operator = get_last_child(operator);
								cur = insert_node(operator, new);
							}
						}
					
						else	
							cur = insert_node(operator, new);
					}
	
					else
						cur = insert_node(operator, new);
				}
			}
		}
		else 
		{
			new = create_node(tokens[*idx], parentheses);

			if(cur == NULL)
				cur = new;

			else if(cur->child_head == NULL){
				cur->child_head = new;
				new->parent = cur;

				cur = new;
			}
			else{

				cur = get_last_child(cur);

				cur->next = new;
				new->prev = cur;

				cur = new;
			}
		}

		*idx += 1;
	}

	return get_root(cur);
}

// change parent's first and second child
node *change_sibling(node *parent)
{
	node *tmp;
	
	// tmp is first node among siblings
	tmp = parent->child_head;

	// change child_head from first child to second child
	parent->child_head = parent->child_head->next;
	// set parent to new child_head
	parent->child_head->parent = parent;
	// delete new child_head's prev node
	parent->child_head->prev = NULL;

	// set old_child_head to new_child_head's next
	parent->child_head->next = tmp;
	// set old_child's prev to new_child_head
	parent->child_head->next->prev = parent->child_head;
	// delete old_child_head's next
	parent->child_head->next->next = NULL;
	// delete old_child head's parent
	parent->child_head->next->parent = NULL;		

	return parent;
}

node *create_node(char *name, int parentheses)
{
	node *new;

	new = (node *)malloc(sizeof(node));
	new->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
	strcpy(new->name, name);

	new->parentheses = parentheses;
	new->parent = NULL;
	new->child_head = NULL;
	new->prev = NULL;
	new->next = NULL;

	return new;
}

int get_precedence(char *op)
{
	int i;

	for(i = 2; i < OPERATOR_CNT; i++){
		if(!strcmp(operators[i].operator, op))
			return operators[i].precedence;
	}
	return false;
}

int is_operator(char *op)
{
	int i;

	for(i = 0; i < OPERATOR_CNT; i++)
	{
		if(operators[i].operator == NULL)
			break;
		if(!strcmp(operators[i].operator, op)){
			return true;
		}
	}

	return false;
}

void print(node *cur)
{
	if(cur->child_head != NULL){
		print(cur->child_head);
		printf("\n");
	}

	if(cur->next != NULL){
		print(cur->next);
		printf("\t");
	}
	printf("%s", cur->name);
}

node *get_operator(node *cur)
{
	if(cur == NULL)
		return cur;

	if(cur->prev != NULL)
		while(cur->prev != NULL)
			cur = cur->prev;

	return cur->parent;
}

node *get_root(node *cur)
{
	if(cur == NULL)
		return cur;

	while(cur->prev != NULL)
		cur = cur->prev;

	if(cur->parent != NULL)
		cur = get_root(cur->parent);

	return cur;
}

node *get_high_precedence_node(node *cur, node *new)
{
	if(is_operator(cur->name))
		if(get_precedence(cur->name) < get_precedence(new->name))
			return cur;

	if(cur->prev != NULL){
		while(cur->prev != NULL){
			cur = cur->prev;
			
			return get_high_precedence_node(cur, new);
		}


		if(cur->parent != NULL)
			return get_high_precedence_node(cur->parent, new);
	}

	if(cur->parent == NULL)
		return cur;
}

node *get_most_high_precedence_node(node *cur, node *new)
{
	node *operator = get_high_precedence_node(cur, new);
	node *saved_operator = operator;

	while(1)
	{
		if(saved_operator->parent == NULL)
			break;

		if(saved_operator->prev != NULL)
			operator = get_high_precedence_node(saved_operator->prev, new);

		else if(saved_operator->parent != NULL)
			operator = get_high_precedence_node(saved_operator->parent, new);

		saved_operator = operator;
	}
	
	return saved_operator;
}

node *insert_node(node *old, node *new)
{
	if(old->prev != NULL){
		new->prev = old->prev;
		old->prev->next = new;
		old->prev = NULL;
	}

	new->child_head = old;
	old->parent = new;

	return new;
}

node *get_last_child(node *cur)
{
	if(cur->child_head != NULL)
		cur = cur->child_head;

	while(cur->next != NULL)
		cur = cur->next;

	return cur;
}

int get_sibling_cnt(node *cur)
{
	int i = 0;

	while(cur->prev != NULL)
		cur = cur->prev;

	while(cur->next != NULL){
		cur = cur->next;
		i++;
	}

	return i;
}

void free_node(node *cur)
{
	if(cur->child_head != NULL)
		free_node(cur->child_head);

	if(cur->next != NULL)
		free_node(cur->next);

	if(cur != NULL){
		cur->prev = NULL;
		cur->next = NULL;
		cur->parent = NULL;
		cur->child_head = NULL;
		free(cur);
	}
}

/* called in make_tokens() */
/* return : is [0-9a-zA-Z] */
/* c : char in s_ans */
int is_character(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/* called in make_tokens */
/* check str is Statement */
/* 0 : wrong answer */
/* 1 : s_answer is normal answer */
/* 2 : s_answer starts with gcc | or datatype */
/* str : student's answer ex) lseek(fd, 15000, 0) */
int is_typeStatement(char *str)
{ 
	char *start;  // index for str
	char str2[BUFLEN] = {0};  // save str with no space
	char tmp[BUFLEN] = {0}; 
	char tmp2[BUFLEN] = {0};  // if student's answer has "gcc", save here 
	int i;	 
	
	start = str;  // points str's head
	strncpy(str2,str,strlen(str));  // copy str to str2
	remove_space(str2);  // remove space in str. ex) lseek(fd,15000,0)

	while(start[0] == ' ')  // make start points non-space char of str
		start += 1;  // ++ index

	if(strstr(str2, "gcc") != NULL)  // str2 has "gcc"
	{
		strncpy(tmp2, start, strlen("gcc"));  // save "gcc" to 3 byte in start
		if(strcmp(tmp2,"gcc") != 0)  // if student's answer doesn't start with gcc
			return 0;  // return 0
		else           // student's answer starts with gcc
			return 2;  // return 2
	}
	
	for(i = 0; i < DATATYPE_SIZE; i++)  // i = 0 ~ < 35
	{
		if(strstr(str2,datatype[i]) != NULL)  // if s_answer has i'th datatype
		{	
			strncpy(tmp, str2, strlen(datatype[i]));  // save first part
			strncpy(tmp2, start, strlen(datatype[i]));  // save first part
			
			if(strcmp(tmp, datatype[i]) == 0)  // if str2 starts with datatype[i]
				if(strcmp(tmp, tmp2) != 0)  // if str and str2's first part is diff
					return 0;  // wrong answer. s_answer has space in datatype
				else
					return 2;  // s_answer starts with datatype
		}

	}
	return 1;  // normal answer

}

int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]) 
{
	int i, j;

	for(i = 0; i < TOKEN_CNT; i++)
	{
		for(j = 0; j < DATATYPE_SIZE; j++)
		{
			if(strstr(tokens[i], datatype[j]) != NULL && i > 0)
			{
				if(!strcmp(tokens[i - 1], "(") && !strcmp(tokens[i + 1], ")") 
						&& (tokens[i + 2][0] == '&' || tokens[i + 2][0] == '*' 
							|| tokens[i + 2][0] == ')' || tokens[i + 2][0] == '(' 
							|| tokens[i + 2][0] == '-' || tokens[i + 2][0] == '+' 
							|| is_character(tokens[i + 2][0])))  
					return i;
			}
		}
	}
	return -1;
}

int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]) 
{
    int i, j;

   
    for(i = 0; i < TOKEN_CNT; i++)
    {
        for(j = 0; j < DATATYPE_SIZE; j++)
        {
            if(!strcmp(tokens[i], "struct") && (i+1) <= TOKEN_CNT && is_character(tokens[i + 1][strlen(tokens[i + 1]) - 1]))  
                    return i;
        }
    }
    return -1;
}

/* called in make_tokens() */
/* return */
/* 0 : str has not * char, */
/* 1 : str is composed with */
/* str : prev token */
int all_star(char *str)
{
	int i;  // index
	int length= strlen(str);  // len of token
	
 	if(length == 0)	// no *
		return 0;
	
	for(i = 0; i < length; i++)
		if(str[i] != '*')  // not * char exists
			return 0;
	return 1;  // only * there

}

int all_character(char *str)
{
	int i;

	for(i = 0; i < strlen(str); i++)
		if(is_character(str[i]))
			return 1;
	return 0;
	
}

int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]) 
{
	int i;
	int j = start - 1;
	int lcount = 0, rcount = 0;
	int sub_lcount = 0, sub_rcount = 0;

	if(start > -1){
		if(!strcmp(tokens[start], "struct")) {		
			strcat(tokens[start], " ");
			strcat(tokens[start], tokens[start+1]);	     

			for(i = start + 1; i < TOKEN_CNT - 1; i++){
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}

		else if(!strcmp(tokens[start], "unsigned") && strcmp(tokens[start+1], ")") != 0) {		
			strcat(tokens[start], " ");
			strcat(tokens[start], tokens[start + 1]);	     
			strcat(tokens[start], tokens[start + 2]);

			for(i = start + 1; i < TOKEN_CNT - 1; i++){
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}

     		j = start + 1;           
        	while(!strcmp(tokens[j], ")")){
                	rcount ++;
                	if(j==TOKEN_CNT)
                        	break;
                	j++;
        	}
	
		j = start - 1;
		while(!strcmp(tokens[j], "(")){
        	        lcount ++;
                	if(j == 0)
                        	break;
               		j--;
		}
		if( (j!=0 && is_character(tokens[j][strlen(tokens[j])-1]) ) || j==0)
			lcount = rcount;

		if(lcount != rcount )
			return false;

		if( (start - lcount) >0 && !strcmp(tokens[start - lcount - 1], "sizeof")){
			return true; 
		}
		
		else if((!strcmp(tokens[start], "unsigned") || !strcmp(tokens[start], "struct")) && strcmp(tokens[start+1], ")")) {		
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			strcpy(tokens[start - lcount + 1], tokens[start + rcount]);
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount]);
				memset(tokens[i + lcount + rcount], 0, sizeof(tokens[0]));
			}


		}
 		else{
			if(tokens[start + 2][0] == '('){
				j = start + 2;
				while(!strcmp(tokens[j], "(")){
					sub_lcount++;
					j++;
				} 	
				if(!strcmp(tokens[j + 1],")")){
					j = j + 1;
					while(!strcmp(tokens[j], ")")){
						sub_rcount++;
						j++;
					}
				}
				else 
					return false;

				if(sub_lcount != sub_rcount)
					return false;
				
				strcpy(tokens[start + 2], tokens[start + 2 + sub_lcount]);	
				for(int i = start + 3; i<TOKEN_CNT; i++)
					memset(tokens[i], 0, sizeof(tokens[0]));

			}
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			strcat(tokens[start - lcount], tokens[start + rcount + 1]);
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount -1; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount +1]);
				memset(tokens[i + lcount + rcount + 1], 0, sizeof(tokens[0]));

			}
		}
	}
	return true;
}

/* called in make_tokens */
/* init 0 */ 
void clear_tokens(char tokens[TOKEN_CNT][MINLEN])
{
	int i;

	for(i = 0; i < TOKEN_CNT; i++)
		memset(tokens[i], 0, sizeof(tokens[i]));
}

/* called in score_blank() */
/* remove right spaces */
/* _str : student's answer */
char *rtrim(char *_str)
{
	char tmp[BUFLEN];  // tmp for _str
	char *end;  // index points tmp's tail

	strcpy(tmp, _str);  // copy
	end = tmp + strlen(tmp) - 1;  // point last char in _str
	while(end != _str && isspace(*end))  // end points first space after answer 
		--end;  // -- index

	*(end + 1) = '\0';  // replace space to \0
	_str = tmp;  // save
	return _str;  // return without no space after chars
}

/* called in score_blank() */
/* remove left spaces */
/* _str : student's answer */
char *ltrim(char *_str)
{
	char *start = _str;  // points _str's head

	while(*start != '\0' && isspace(*start))  // start points last space before answer
		++start;  // ++ index
	_str = start;  // save
	return _str;  // return without no space before chars
}

char* remove_extraspace(char *str)
{
	int i;
	char *str2 = (char*)malloc(sizeof(char) * BUFLEN);
	char *start, *end;
	char temp[BUFLEN] = "";
	int position;

	if(strstr(str,"include<")!=NULL){
		start = str;
		end = strpbrk(str, "<");
		position = end - start;
	
		strncat(temp, str, position);
		strcat(temp, " ");
		strncat(temp, str + position, strlen(str) - position + 1);

		str = temp;		
	}
	
	for(i = 0; i < strlen(str); i++)
	{
		if(str[i] ==' ')
		{
			if(i == 0 && str[0] ==' ')
				while(str[i + 1] == ' ')
					i++;	
			else{
				if(i > 0 && str[i - 1] != ' ')
					str2[strlen(str2)] = str[i];
				while(str[i + 1] == ' ')
					i++;
			} 
		}
		else
			str2[strlen(str2)] = str[i];
	}

	return str2;
}



/* called in is_typeStatement */
/* remove space in str */
/* str : copy of student's answer */
void remove_space(char *str)
{
	char* i = str;  // points str's head
	char* j = str;  // points str's head
	
	while(*j != 0)  // while in str
	{
		*i = *j++;  // overwrite j'th to i'th
		if(*i != ' ')  // if i'th char is space, overwrite next time
			i++;
	}
	*i = 0;  // str ends with \0
}

/* called in score_blank */
/* check if student's answer is grammarly right abount ( ) */
/* std : student's ans. ex) lseek(fd, 15000, 0) */
/* return : 0(wrong) or 1(right) */
int check_brackets(char *str)
{
	char *start = str;  // point str's head
	int lcount = 0, rcount = 0;
	
	while(1){  // check str
		if((start = strpbrk(start, "()")) != NULL){  // if str has ( or )
			if(*(start) == '(')  // current ch is (
				lcount++;  // ( count ++
			else
				rcount++;  // ) count ++

			start += 1;    // find ( or ) in the rest 		
		}
		else
			break;  // no more ( or ), break
	}

	if(lcount != rcount)  // check ( and ) pair
		return 0;  // wrong answer in grammarly
	else 
		return 1;  // right answer in grammarly
}

int get_token_cnt(char tokens[TOKEN_CNT][MINLEN])
{
	int i;
	
	for(i = 0; i < TOKEN_CNT; i++)
		if(!strcmp(tokens[i], ""))
			break;

	return i;
}
