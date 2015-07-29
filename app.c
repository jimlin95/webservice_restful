#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mongoose.h"
#include "cJSON.h"
#include "sqlite3.h"
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
int callback(void *, int, char **, char **);
static int handle_request(struct mg_connection *conn)
{
    char * json_data;
    cJSON *root;
    char *json;
    char *title;
    char *description;
    int completed;
    if (!strcmp(conn->request_method, "POST") && !strncmp(conn->uri, "/api/v1",strlen("/api/v1"))) 
    {
        printf("api v1\n");
        json_data = get_post_data(conn);
        if(json_data == NULL)
        {
            printf("Error json data: NULL\n");
            return MG_TRUE;   // Tell mongoose to close this connection
        }
        printf("input json data: %s\n", json_data);
        root = cJSON_Parse(json_data);
        description = cJSON_GetObjectItem(root,"description")->valuestring; 
        title = cJSON_GetObjectItem(root,"title")->valuestring; 
        completed = cJSON_GetObjectItem(root,"completed")->valueint; 
        printf("title = %s,description = %s, completed = %d\n",title,description,completed);
        json = cJSON_Print(root);
        write_json_result(conn, 0, json);
        free(json_data);
        return MG_TRUE;   // Tell mongoose to close this connection
    }
    else if (!strcmp(conn->request_method, "GET") && !strncmp(conn->uri, "/api/v1",strlen("/api/v1"))) 
    {
        sqlite3 *db;
        char *err_msg = 0; 
        char sql[100];
        int id;
        char response_buff[100];
        printf("get api v1\n");
        if(!strncmp(conn->uri,"/api/v1/cars",strlen("/api/v1/cars")))
        {
            sscanf(conn->uri,"/api/v1/cars/%d/",&id);
            int rc = sqlite3_open("quanta.db", &db);
            if (rc != SQLITE_OK) 
            {
            
                fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
                sqlite3_close(db);        
                return MG_TRUE;   // Tell mongoose to close this connection
            }
            sprintf(sql,"SELECT * FROM Cars WHERE Id = %d",id);
            rc = sqlite3_exec(db,sql, callback, response_buff, &err_msg);
        
            if (rc != SQLITE_OK ) {
                
                fprintf(stderr, "Failed to select data\n");
                fprintf(stderr, "SQL error: %s\n", err_msg);

                sqlite3_free(err_msg);
                sqlite3_close(db);
                
                return MG_TRUE;   // Tell mongoose to close this connection
            } 
            else
            {
                mg_printf_data(conn,"%s",response_buff);
                return MG_TRUE;   // Tell mongoose to close this connection
            }
            sqlite3_close(db);
        }
        else
        {
            mg_printf_data(conn, "%s","Not Support");
            return MG_TRUE;   // Tell mongoose to close this connection
        }
    }
    else 
    {
        printf("else %d\n",__LINE__);
        mg_printf_data(conn, "%s","Not Support");
        return MG_TRUE;   // Tell mongoose to close this connection
    }
}
int callback(void *response, int argc, char **argv,char **azColName) 
{
    
    cJSON *root,*fmt;  
    int i; 
    root=cJSON_CreateObject();    
    cJSON_AddItemToObject(root, "car",fmt=cJSON_CreateObject());
    for (i = 0; i < argc; i++) 
    {

        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        cJSON_AddStringToObject(fmt,azColName[i],argv[i]);
    }
    strcpy((char *)response,cJSON_PrintUnformatted(root));
    //printf("%s",cJSON_PrintUnformatted(root));
    cJSON_Delete(root);
    return 0;
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
            //printf("event_handler , MG_AUTH\n");
            return MG_TRUE;
        case MG_REQUEST:  
            //printf("event_handler , MG_REQUEST\n");
            return handle_request(conn);
        case MG_RECV:     
            //printf("event_handler , MG_RECV\n");
            return handle_recv(conn);
        case MG_CLOSE:    
            //printf("event_handler , MG_CLOSE\n");
            return handle_close(conn);
        default:          
         //   printf("event_handler , default\n");
            return MG_FALSE;
    }
}
int db_insert(void)
{
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("quanta.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return 1;
    }
    
    char *sql = "DROP TABLE IF EXISTS Cars;" 
                "CREATE TABLE Cars(Id INT, Name TEXT, Price INT);" 
                "INSERT INTO Cars VALUES(1, 'Audi', 52642);" 
                "INSERT INTO Cars VALUES(2, 'Mercedes', 57127);" 
                "INSERT INTO Cars VALUES(3, 'Skoda', 9000);" 
                "INSERT INTO Cars VALUES(4, 'Volvo', 29000);" 
                "INSERT INTO Cars VALUES(5, 'Bentley', 350000);" 
                "INSERT INTO Cars VALUES(6, 'Citroen', 21000);" 
                "INSERT INTO Cars VALUES(7, 'Hummer', 41400);" 
                "INSERT INTO Cars VALUES(8, 'Volkswagen', 21600);";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return 1;
    } 
    
    sqlite3_close(db);
    
    return 0;
    
}

int db_init(void)
{
    sqlite3 *db;
    sqlite3_stmt *res;
    int rc = sqlite3_open("quanta.db", &db);
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    rc = sqlite3_prepare_v2(db, "SELECT SQLITE_VERSION()", -1, &res, 0);    
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return 1;
    }    
    
    rc = sqlite3_step(res);
    
    if (rc == SQLITE_ROW) {
        printf("%s\n", sqlite3_column_text(res, 0));
    }
    
    sqlite3_finalize(res);
    sqlite3_close(db);
    
    return 0;
}

int main(void)
{
    struct mg_server *server = mg_create_server(NULL, event_handler);
    //printf("%s\n", sqlite3_libversion()); 
    //db_init();
    //db_insert();
    mg_set_option(server, "document_root", ".");
    mg_set_option(server, "listening_port", "8080");

    printf("web service is working now\n");
    for (;;) 
    {
        mg_poll_server(server, 1000);  // Infinite loop, Ctrl-C to stop
    }
    mg_destroy_server(&server);

    return 0;
}
