#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>


int check_element(xmlNode * a_node, char* src_ip, char* src_asn)
{
    xmlNode *cur_node = NULL;
	bool flag1 = false,flag2 = false;
	char buffer[128],buffer2[128];
	
    for (cur_node = a_node; cur_node; cur_node = cur_node->next) 
	{
		if (cur_node->type == XML_ELEMENT_NODE) 
		{
		     //printf("node type: Element, name: %s\n",cur_node->name);
			if(xmlStrcmp(cur_node->name,(const xmlChar*)"ADDRESS")==0 && xmlStrcmp(cur_node->children->content,(const xmlChar*)src_ip)==0 && xmlStrcmp(cur_node->parent->name,(const xmlChar*)"SRC_ADDR")==0)
			{
					//printf("111%s\n",cur_node->children->content);
					//printf("222%s\n",cur_node->parent->name);
					flag1 = true;
			}
			else if(xmlStrcmp(cur_node->name,(const xmlChar*)"SRC_AS")==0 && xmlStrcmp(cur_node->children->content,(const xmlChar*)src_asn)==0)
			{
				//printf("333%s\n",cur_node->children->content);
				flag2 = true;
			}
				
		}
		int returned = check_element(cur_node->children,src_ip,src_asn);
		flag1 = flag1 || returned&1;
		flag2 = flag2 || returned&2;
		
    }
	int result = 0;
	if(flag1)
		result |=1;
	if(flag2)
		result |=2;
	return result;
}

bool check_values(xmlNode * a_node, char* src_ip, char* src_asn)
{
	int result = check_element(a_node,src_ip,src_asn);
	//printf("%d\n",result);
	/*if the result contains both the correct source ip address and AS number*/
	if(result == 3)
		return true;
	else 
		return false;
}
/*void print_element(xmlNode * a_node)
{
	xmlNode *cur_node = NULL;
	
    for (cur_node = a_node; cur_node; cur_node = cur_node->next) 
	{
		if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if(xmlStrcmp(cur_node->name,(const xmlChar*)"TIME")==0)
			{
				//printf("%s\n",cur_node->children->name);
				printf("%s\n",xmlGetProp(cur_node,(const xmlChar*)"timestamp"));
			}
			else if(xmlStrcmp(cur_node->name,(const xmlChar*)"OCTET_MSG")==0)
			{
				printf("%s\n",cur_node->children->children->content);
			}
		}
		print_element(a_node->children);
	}
}*/
int main(int argc, char **argv)
{
   xmlDoc *doc = NULL;
   xmlNode *root_element = NULL;

   if (argc != 4)  return(1);

   /*parse the file and get the DOM */
    if ((doc = xmlReadFile(argv[1], NULL, 0)) == NULL){
       printf("error: could not parse file %s\n", argv[1]);
       exit(-1);
    }

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);

	/*If it contains the correct source ip and as number then the TIMESTAMP and OCTET will be printed*/
    //if(check_values(root_element,argv[2],argv[3]))
		//print_element(root_element);
	
  

	if(check_values(root_element,argv[2],argv[3])){
		xmlFreeDoc(doc);       // free document
    	xmlCleanupParser();    // Free globals
		return 1;
	}
	else{
		xmlFreeDoc(doc);       // free document
   		xmlCleanupParser();    // Free globals
		return 0;
	}
	
}
