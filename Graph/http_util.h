#ifndef __HTTP_CLIENT__
#define __HTTP_CLIENT__



#ifdef __cplusplus
extern "C" {
#endif


enum
{
    STATE_NONE = 0 ,
    STATE_HTTP,         //HTTP/1.1 200 OK    /*HTTP*/
    STATE_MAJOR_VERSION,      //HTTP/1.1 200 OK    /*1.1*/
    STATE_MINOR_VERSION,
    STATE_STATUSCODE,   //HTTP/1.1 200 OK    /*200*/
    STATE_RESULT,       //HTTP/1.1 200 OK    /*OK*/

    STATE_ENTER,  //\r
    STATE_EOL,    //\n


    STATE_HEAD_KEY, //
    STATE_HEAD_VALUE,

    STATE_ENTER_2,      //\r\n\r
    STATE_EOF_2,        //\r\n\r\n
};

//#ifndef bool 
//#define bool int 
//#endif

#define PARSE_BUFF_SIZE 1024
struct http_client
{
    char* url;
    int port;
    char* host;
    char* path;
    void *user_data[10];

    int (*parse_url)(struct  http_client *httpc, const char* url);
    char* (*request_string)(struct http_client* httpc);
    void (*add_request_header)(struct http_client* httpc,char* key,char* value);
    int (*process_data)(struct http_client* httpc, char * buff,int buffsize);
    int (*finished)(struct http_client* httpc);


    char parse_buffer[PARSE_BUFF_SIZE];

    void *pfn;
    void *pfn_user_data;

    int parse_state;
    char tmp_string[2048];
    char* tmp_head_key;
    int available_index ;


    int socket;


    int status_code;
    char* location;
    int content_len;

    int predict_content_len;
    char* content_data;
    int data_len;



};



struct  http_client *new_http_client();
void delete_http_client(struct  http_client* httpc);
int parse_data(struct http_client *httpc, char * buff,int buffsize);

#ifdef __cplusplus
} //extern "C" {
#endif


#endif // __HTTP_CLIENT__
