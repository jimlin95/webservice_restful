#include <string.h>
#include <stdio.h>
#include "mongoose.h"
#include "cJSON.h"
/*
    get json_data value form post data.
*/
char * get_post_data(struct mg_connection *conn)
{
    char *buff = NULL;

    size_t buf_len;
    const char *cl;

    cl = mg_get_header(conn, "Content-Length");
    if (!strcmp(conn->request_method, "POST") && cl != NULL)
    {
        buf_len = atoi(cl)+1;
        buff = malloc(buf_len);
        memset(buff,0,buf_len);
        memcpy(buff,conn->content,conn->content_len);
    }
    return buff; /*should free outside*/
}
int write_json_result(struct mg_connection *conn, int code, char * msg)
{
    char * buff;
    int ret;
    cJSON *croot;
    croot = cJSON_CreateObject();
    cJSON_AddItemToObject(croot,"Code",cJSON_CreateBool(code));
    cJSON_AddItemToObject(croot,"Msg",cJSON_CreateString(msg));
    buff =  cJSON_Print(croot);
    ret = mg_printf_data(conn,"Result=%s",buff);
    cJSON_Delete(croot);
    return ret;
}

static int handle_request(struct mg_connection *conn)
{
    char * json_data;
    cJSON *root;
    char *json;
    char *title;
    char *description;
    int completed;
    int ret=0;
    if (!strcmp(conn->request_method, "POST") && !strcmp(conn->uri, "/api/v1")) 
    {
        printf("api v1\n");
        json_data = get_post_data(conn);
        if(json_data == NULL)
        {
            printf("Error json data: NULL\n");
            return NULL;
        }
        printf("input json data: %s\n", json_data);
        root = cJSON_Parse(json_data);
        description = cJSON_GetObjectItem(root,"description")->valuestring; 
        title = cJSON_GetObjectItem(root,"title")->valuestring; 
        completed = cJSON_GetObjectItem(root,"completed")->valueint; 
        printf("title = %s,description = %s, completed = %d\n",title,description,completed);
        json = cJSON_Print(root);
        ret = write_json_result(conn, 0, json);
        free(json_data);
        return MG_TRUE;   // Tell mongoose to close this connection
    }
    else 
    {
        printf("else %d\n",__LINE__);
        mg_printf_data(conn, "%s",
                   "<html><body>Upload example."
                   "<form method=\"POST\" action=\"/upload\" "
                   "  enctype=\"multipart/form-data\">"
                   "<input type=\"file\" name=\"file\" /> <br/>"
                   "<input type=\"submit\" value=\"Upload\" />"
                   "</form></body></html>");
        return MG_TRUE;   // Tell mongoose to close this connection
    }
}

// Mongoose sends MG_RECV for every received POST chunk.
// When last POST chunk is received, Mongoose sends MG_REQUEST, then MG_CLOSE.
static int handle_recv(struct mg_connection *conn) 
{
    FILE *fp = (FILE *) conn->connection_param;
    // Open temporary file where we going to write data
    if (fp == NULL && ((conn->connection_param = fp = tmpfile())) == NULL) 
    {
        return -1;  // Close connection on error
    }

    // Return number of bytes written to a temporary file: that is how many
    // bytes we want to discard from the receive buffer
    return fwrite(conn->content, 1, conn->content_len, fp);
}

// Make sure we free all allocated resources
static int handle_close(struct mg_connection *conn) 
{
    if (conn->connection_param != NULL) 
    {
        fclose((FILE *) conn->connection_param);
        conn->connection_param = NULL;
    }
    return MG_TRUE;
}
static int event_handler(struct mg_connection *conn, enum mg_event ev) 
{
    switch (ev) 
    {
        case MG_AUTH:     
            printf("event_handler , MG_AUTH\n");
            return MG_TRUE;
        case MG_REQUEST:  
            printf("event_handler , MG_REQUEST\n");
            return handle_request(conn);
        case MG_RECV:     
            printf("event_handler , MG_RECV\n");
            return handle_recv(conn);
        case MG_CLOSE:    
            printf("event_handler , MG_CLOSE\n");
            return handle_close(conn);
        default:          
         //   printf("event_handler , default\n");
            return MG_FALSE;
    }
}

int main(void)
{
    struct mg_server *server = mg_create_server(NULL, event_handler);
    mg_set_option(server, "document_root", ".");
    mg_set_option(server, "listening_port", "8080");

    printf("APP start\n");
    for (;;) 
    {
        mg_poll_server(server, 1000);  // Infinite loop, Ctrl-C to stop
    }
    mg_destroy_server(&server);

    return 0;
}
