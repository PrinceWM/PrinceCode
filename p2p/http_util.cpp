//#ifdef WIN32
#include "stdafx.h"
//#endif

#define _CRT_SECURE_NO_DEPRECATE 
#include "http_util.h"
#include <stdlib.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>


#ifdef _MSC_VER
int strcasecmp(char *s1, char *s2)
{
	while  (toupper((unsigned char)*s1) == toupper((unsigned char)*s2++))
		if (*s1++ == '\0') return 0;
	return(toupper((unsigned char)*s1) - toupper((unsigned char)*--s2));
}

int strncasecmp(const char *s1, const char *s2, register int n)
{
	while (--n >= 0 && toupper((unsigned char)*s1) == toupper((unsigned char)*s2++))
		if (*s1++ == '\0')  return 0;
	return(n < 0 ? 0 : toupper((unsigned char)*s1) - toupper((unsigned char)*--s2));
}
int isblank(char c)
{
	return ((c==' ') || (c == '\t'));
}
#endif

static int parse_url(struct  http_client *httpc, const char* weburl)
{
    char *pc,c;
	char* urlcopy = strdup(weburl);

	char* url = urlcopy;

    httpc->port=80;
    if (httpc->host) {
        free(httpc->host);
        httpc->host=0;
    }

    if (httpc->path) {
        free(httpc->path);
        httpc->path=0;
    }

    if (strncasecmp("http://",url,7) ) {
        printf("invalid url (must start with 'http://') ,url:%s\n",url);
		free(url);
        return -1;
    }
    url+=7;
    for (pc=url,c=*pc; (c && c!=':' && c!='/');) c=*pc++;
    *(pc-1)=0;
    if (c==':')
    {
        if (sscanf(pc,"%d",&(httpc->port))!=1)
        {
            printf("invalid port in url\n");
			free(url);
            return -1;
        }
        for (pc++; (*pc && *pc!='/') ; pc++) ;
        if (*pc) pc++;
    }

    httpc->host=strdup(url);
    httpc->path= strdup ( c ? pc : "") ;

    printf("host=(%s), port=%d, filename=(%s)\n",
            httpc->host,httpc->port,httpc->path);

	free(urlcopy);

    return 0;

}

char* request_string(struct http_client* httpc)
{
    char *pheader = (char*)malloc(2048);
    memset(pheader,0,2048);

    sprintf(pheader,
         "%s /%s HTTP/1.1\015\012" \
         "Host: %s:%d\015\012" \
        "User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/27.0.1453.94 Safari/537.36\015\012"\
         "Connection: keep-alive\015\012\015\012",
         "GET",
         httpc->path,
         httpc->host,
            httpc->port
         );

    return pheader;
}
int process_data(struct http_client *httpc, char * buff,int buffsize)
{
	if(httpc->parse_state != STATE_EOF_2)
	{
		return parse_data(httpc,buff,buffsize);
	}
	else
	{
		//printf("process_data  ,httpc->data_len:%d ,httpc->content_len:%d\n",httpc->data_len,httpc->content_len);fflush(stdout);


		if(httpc->predict_content_len > 0 && httpc->content_len == 0 )
		{
			if( httpc->predict_content_len - httpc->data_len < buffsize)
			{
				char* tempbuff= 0;
				httpc->predict_content_len = httpc->predict_content_len * 2 < httpc->data_len + buffsize ?
					httpc->data_len + buffsize:httpc->predict_content_len * 2;
				tempbuff= (char *)malloc(httpc->predict_content_len + 1);
				memset(tempbuff,0,httpc->predict_content_len + 1);
				memcpy(tempbuff,httpc->content_data,httpc->data_len);
				free(httpc->content_data);
				httpc->content_data = tempbuff;
			}
		}

		memcpy(httpc->content_data + httpc->data_len,buff,buffsize);
		httpc->data_len += buffsize;



		if(strcmp(httpc->content_data + httpc->data_len - 2,"\r\n") == 0 )
		{
			httpc->content_len = httpc->data_len;
		}



		if(httpc->content_len == httpc->data_len )
		{
			printf("process_data  ,has finished\n");
			fflush(stdout);
		}

		return buffsize;
	}
}
int parse_data(struct http_client *httpc, char * buff,int buffsize)
{
    int iRead = 0;
    for(;iRead < buffsize;iRead ++)
    {
        char c = buff[iRead];
        switch(httpc->parse_state)
        {
			case STATE_NONE:
			{
				if(c == 'H' || c == 'T' || c == 'P')
				{
					continue;
				}
				else if(c == '/')
				{
					httpc->parse_state =STATE_MAJOR_VERSION;
					continue;
				}
				break;
			}
			case STATE_MAJOR_VERSION:
			{
				if(isdigit(c))
				{
					continue;
				}
				else if(c == '.')
				{
					httpc->parse_state =STATE_MINOR_VERSION;
					continue;
				}
				break;
			}
			case STATE_MINOR_VERSION:
			{
				if(isdigit(c))
				{
					continue;
				}
				else if(isblank(c))
				{
					httpc->parse_state = STATE_STATUSCODE;
					memset(httpc->tmp_string,0,sizeof(httpc->tmp_string));
					httpc->available_index = 0;
					continue;
				}
				break;
			}
			case STATE_STATUSCODE:
			{
				if(isdigit(c))
				{
					httpc->tmp_string[httpc->available_index++] = c;
					continue;
				}
				else if(isblank(c))
				{
					httpc->status_code = atoi(httpc->tmp_string);
					httpc->parse_state = STATE_RESULT;
					continue;
				}
				break;
			}
			case STATE_RESULT:
			{
				if(isblank(c) || isalnum(c) )
					continue;
				else if(c == '\r')
				{
					httpc->parse_state = STATE_ENTER;
					continue;
				}
				break;
			}
			case STATE_ENTER:
			{
				if(c == '\n')
				{
					httpc->parse_state = STATE_EOL;
					continue;
				}
				break;
			}
			case STATE_EOL:
			{
				if(isalnum(c))
				{
					httpc->parse_state = STATE_HEAD_KEY;
					memset(httpc->tmp_string,0,sizeof(httpc->tmp_string));
					httpc->available_index = 0;
					httpc->tmp_string[httpc->available_index++] = c;
					continue;
				}
				else if(c == '\r')
				{
					httpc->parse_state = STATE_ENTER_2;

				}
				break;
			}
			case STATE_HEAD_KEY:
			{
				if(isblank(c) || c == ':')
				{
	//                log("%s\n",httpc->tmp_string);fflush(stdout);

					httpc->tmp_head_key = strdup(httpc->tmp_string);
					httpc->parse_state = STATE_HEAD_VALUE;
					memset(httpc->tmp_string,0,sizeof(httpc->tmp_string));
					httpc->available_index = 0;
					continue;
				}
				else if(c != ':')
				{
					httpc->tmp_string[httpc->available_index++] = c;
					continue;
				}
				break;
			}
			case STATE_HEAD_VALUE:
			{
				if(c!='\r')
				{
					httpc->tmp_string[httpc->available_index++] = c;
					continue;
				}
				else
				{
					printf("%s\n",httpc->tmp_string);fflush(stdout);
					if(strncasecmp("Location",httpc->tmp_head_key,8) == 0 )
					{
						httpc->location = strdup(httpc->tmp_string);
					}
					else if(strncasecmp("Content-Length",httpc->tmp_head_key,strlen("Content-Length")) == 0 )
					{
						httpc->content_len = atoi(httpc->tmp_string);
						httpc->content_data = (char*)malloc(httpc->content_len + 1);
						memset(httpc->content_data,0,httpc->content_len + 1);
					}
					//else if(strncasecmp("SINA-TS",httpc->tmp_string,strlen("SINA-TS")) == 0 )
					//{

					//	int i;
					//	i++;
					//}

					httpc->parse_state = STATE_ENTER;
					continue;
				}
				break;
			}
			case STATE_ENTER_2:
			{
				if(c == '\n')
				{
					httpc->parse_state = STATE_EOF_2;
					iRead ++;

					if(httpc->content_data == 0 && httpc->content_len == 0 ) //
					{
						httpc->predict_content_len = 200 < (buffsize - iRead)?(buffsize - iRead):200 ;
						httpc->content_data = (char *)malloc(httpc->predict_content_len + 1);
						memset(httpc->content_data,0,httpc->predict_content_len + 1);
					}

					if(iRead < buffsize)
					{
						memcpy(httpc->content_data, buff /*httpc->parse_buffer*/ + iRead,buffsize - iRead);
						httpc->data_len = buffsize - iRead;

						if(strcmp(httpc->content_data + httpc->data_len - 2,"\r\n") == 0 )
						{
							httpc->content_len = httpc->data_len;
						}

						/*log("STATE_ENTER_2  ,httpc->data_len:%d ,httpc->content_len:%d ,iRead:%d\n",
							   httpc->data_len,httpc->content_len,iRead);fflush(stdout);*/
					}
				}
				break;

			}
			default:
			{
				return -1;
			}
        }//switch 

        if(httpc->parse_state == STATE_EOF_2)
        {
            break;
        }

    }//for
    return  iRead ;
}





int finished(struct http_client* httpc)
{

    return (httpc->content_len == httpc->data_len && httpc->content_len > 0 ) ;
}

struct  http_client *new_http_client()
{
    struct  http_client *httpc = (http_client *)malloc(sizeof(struct http_client));
    memset(httpc,0,sizeof(struct http_client));

    httpc->parse_url = parse_url;
    httpc->request_string = request_string;
    httpc->add_request_header = 0;
    httpc->process_data = process_data;
    httpc->finished = finished;


    return httpc;
}

void delete_http_client(struct  http_client* httpc)
{
    if(httpc->tmp_head_key)
        free(httpc->tmp_head_key);
    if(httpc->location)
        free(httpc->location);
    if(httpc->content_data)
        free(httpc->content_data);
	if(httpc->host)
		free(httpc->host);
	if(httpc->path)
		free(httpc->path);

    free(httpc);

}
