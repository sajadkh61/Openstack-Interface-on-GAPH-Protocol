#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "curl-master/include/curl/curl.h"
#include "jsmn/jsmn.h"
 
// Globals
char tenantid[20];
 
struct auth_info {
    char tokenid[50];
    char tenantid[20];
    char region[4];
    int  filled;
}; 
 
struct string {
    char *ptr;
    size_t len;
};

void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len+1);
    if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(1);
    }
    s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
    size_t new_len = s->len + size*nmemb;
    s->ptr = realloc(s->ptr, new_len+1);
    if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(1);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size*nmemb;
}


//============================== functions ===========================
  
// ======================== Get Token 
struct auth_info getAuthInfo()
{
    CURL *curl;
    CURLcode res;
    struct auth_info Ainfo;
    Ainfo.filled = 0;
    Ainfo.tokenid[0] = '\0';
    Ainfo.tenantid[0] = '\0';
    Ainfo.region[0] = '\0';
    curl = curl_easy_init();
    if(curl) {
        struct string s;
        init_string(&s);
         
        curl_easy_setopt(curl, CURLOPT_URL, "https://identity.api.rackspacecloud.com/v2.0/tokens");
        /* example.com is redirected, so we tell libcurl to follow redirection */ 
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        struct curl_slist *headers = NULL;
        
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "User-Agent: python-novaclient");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"auth\": {\"RAX-KSKEY:apiKeyCredentials\": {\"username\": \"opencloudlab\", \"apiKey\": \"e32b5097596174ba575d141546e0b427\"}}}");


     
        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK)
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));


        curl_easy_cleanup(curl);
        //printf("%s\n length=%d", s.ptr, s.len);
        // get token id   
        
        int r;
        jsmn_parser p;
        jsmntok_t tokens[s.len];

        jsmn_init(&p);
        r = jsmn_parse(&p, s.ptr, s.len, tokens, s.len);

        int i;
        int len = 0;
        int got_id = 0;
        int got_tenant = 0;
        int got_region = 0;
        
        for (i = 1; tokens[i].end < tokens[0].end && tokens[i].end > 0; i++) {
            if (tokens[i].type == JSMN_STRING || tokens[i].type == JSMN_PRIMITIVE) {
                //printf("%.*s--\n", tokens[i].end - tokens[i].start, s.ptr + tokens[i].start);
                if(strncmp(s.ptr + tokens[i].start, "id", tokens[i].end - tokens[i].start ) == 0 && !got_id)    // tokenid
                {
                    i++;
                    strncpy(Ainfo.tokenid, s.ptr + tokens[i].start, tokens[i].end - tokens[i].start);
                    Ainfo.tokenid[tokens[i].end - tokens[i].start] = '\0';
                    len = tokens[i].end - tokens[i].start;
                    got_id = 1;
                    //printf("%.*s--\n", tokens[i].end - tokens[i].start, s.ptr + tokens[i].start);
                    //printf("%s\n", id);                    
                }
                else if(strncmp(s.ptr + tokens[i].start, "tenant", tokens[i].end - tokens[i].start ) == 0 && !got_tenant)
                {
                    i += 3;
                    strncpy(Ainfo.tenantid, s.ptr + tokens[i].start, tokens[i].end - tokens[i].start);
                    Ainfo.tenantid[tokens[i].end - tokens[i].start] = '\0';       
                    got_tenant = 1;
                }
                else if(strncmp(s.ptr + tokens[i].start, "region", tokens[i].end - tokens[i].start ) == 0 && !got_region)
                {
                    i += 1;
                    strncpy(Ainfo.region, s.ptr + tokens[i].start, tokens[i].end - tokens[i].start);
                    Ainfo.region[tokens[i].end - tokens[i].start] = '\0';
                    got_region = 1;
                }
            }
        }
        if(Ainfo.tokenid[0] != '\0' && Ainfo.tenantid[0] != '\0' && Ainfo.region[0] != '\0')    Ainfo.filled = 1;
    }
    return Ainfo;
}


//=============================== Create Server
int createServer(struct auth_info Ainfo, char* name)
{
    CURL *curl;
    CURLcode res;
    int success = 0;
    
    curl = curl_easy_init();
    if(curl) {
        struct string s;
        init_string(&s);
        char tmp[70] = "X-Auth-Token: ";
        strcat(tmp, Ainfo.tokenid);
        
        char url[200];
        strcpy(url, "https://");
        strcat(url, Ainfo.region);
        strcat(url, ".servers.api.rackspacecloud.com/v2/");
        strcat(url, Ainfo.tenantid);
        strcat(url, "/servers");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        /* tell libcurl to follow redirection */ 
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "User-Agent: python-novaclient");
        headers = curl_slist_append(headers, "X-Auth-Project-Id: opencloudlab");
        headers = curl_slist_append(headers, tmp);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        char* prename = "{\"server\": {\"name\": \"";
        char* postname = "\", \"imageRef\": \"0f4a93c6-a08c-4cf3-9fec-abe968f06892\", \"flavorRef\": \"2\", \"max_count\": 1, \"min_count\": 1, \"networks\": [{\"uuid\": \"00000000-0000-0000-0000-000000000000\"}, {\"uuid\": \"11111111-1111-1111-1111-111111111111\"}]}}";
        int nlen = strlen(prename) + strlen(name) + strlen(postname);
        char* fullname = malloc(nlen+1);
        strcpy(fullname, prename);
        strcat(fullname, name);
        strcat(fullname, postname);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fullname);

         
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            success = 0;
        }
        else success = 1;
        //printf("%s\n", s.ptr);        // response

        /* always cleanup */ 
        curl_easy_cleanup(curl);
  }
  
  return success;
}

// curl -i 'https://ord.servers.api.rackspacecloud.com/v2/863421/servers/detail' -X GET -H "Accept: application/json" -H "User-Agent: python-novaclient" -H "X-Auth-Project-Id: opencloudlab" -H "X-Auth-Token: {SHA1}ecfbd5a1d193d288878dd7b4d727c6ab0c9f8308"

struct string list(struct auth_info Ainfo)
{
    CURL *curl;
    CURLcode res;
    int success = 0;
    struct string s;
    init_string(&s);
    curl = curl_easy_init();
    if(curl) {
        char tmp[70] = "X-Auth-Token: ";
        strcat(tmp, Ainfo.tokenid);
        
        char url[200];
        strcpy(url, "https://");
        strcat(url, Ainfo.region);
        strcat(url, ".servers.api.rackspacecloud.com/v2/");
        strcat(url, Ainfo.tenantid);
        strcat(url, "/servers/detail");
        
        curl_easy_setopt(curl, CURLOPT_URL, url);
        /* example.com is redirected, so we tell libcurl to follow redirection */ 
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "User-Agent: python-novaclient");
        headers = curl_slist_append(headers, "X-Auth-Project-Id: opencloudlab");
        headers = curl_slist_append(headers, tmp);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

         
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            success = 0;
        }
        else success = 1;
        //printf("%s\n", s.ptr);        // response

        /* always cleanup */ 
        curl_easy_cleanup(curl);
  }
  
  return s;
}


void printList(struct string s)
{
    if(s.len <= 0)  return;
    
    int r;
    jsmn_parser p;
    jsmntok_t tokens[s.len];
    jsmn_init(&p);
    r = jsmn_parse(&p, s.ptr, s.len, tokens, s.len);
    
    struct server* servers;
    int servers_count = 0;
    
    int i, j;
    int got_href = 0;
    int count = 0;
    jsmntok_t *x;
    int got_public = 0;
    int got_public_addr = 0;
    int got_id = 0;
    
    for (i = 1; tokens[i].end < tokens[0].end && tokens[i].end > 0; i++) {
        if (tokens[i].type == JSMN_STRING || tokens[i].type == JSMN_PRIMITIVE) {            
            if(strncmp(s.ptr + tokens[i].start, "public", tokens[i].end - tokens[i].start ) == 0 && (tokens[i].end - tokens[i].start) > 0)            
                got_public = 1;
            else if(strncmp(s.ptr + tokens[i].start, "addr", tokens[i].end - tokens[i].start ) == 0 && (tokens[i].end - tokens[i].start) > 0 && got_public)   
            {
                got_public_addr++;
                if(got_public_addr > 1) {           // new server                    
                    i++;
                    int len = tokens[i].end - tokens[i].start;
                    char* ipp = malloc(len+1);
                    strncpy(ipp, s.ptr + tokens[i].start, len);
                    ipp[len] = '\0';
                
                    if(got_public_addr == 2)
                    {
                        
                        printf("IP (public): %s\n", ipp);
                    }
                    else if(got_public_addr == 3)
                    {
                        got_public_addr = 0;
                        got_public = 0;
                        printf("IP (private): %s\n", ipp);
                    }
                }                    
            }            
            else if(strncmp(s.ptr + tokens[i].start, "id", tokens[i].end - tokens[i].start ) == 0 && (tokens[i].end - tokens[i].start) > 0)
            {
                got_id++;
                if(got_id >= 3)
                {                
                    i++;
                    int len = tokens[i].end - tokens[i].start;
                    char* id = malloc(len+1);
                    strncpy(id, s.ptr + tokens[i].start, len);
                    id[len] = '\0';
                    printf("ID: %s\n", id);
                    got_id = 0;
                }
            }
            else if(strncmp(s.ptr + tokens[i].start, "status", tokens[i].end - tokens[i].start ) == 0 && (tokens[i].end - tokens[i].start) > 0)
            {
                i++;
                int len = tokens[i].end - tokens[i].start;
                char* status = malloc(len+1);
                strncpy(status, s.ptr + tokens[i].start, len);
                status[len] = '\0';
                count++;
                printf("\nServer #%d\n", count);
                printf("----------\n");
                printf("Status: %s\n", status);
            }
            else if(strncmp(s.ptr + tokens[i].start, "name", tokens[i].end - tokens[i].start ) == 0 && (tokens[i].end - tokens[i].start) > 0)
            {
                i++;
                int len = tokens[i].end - tokens[i].start;
                char* name = malloc(len+1);
                strncpy(name, s.ptr + tokens[i].start, len);
                name[len] = '\0';
                printf("Name: %s\n", name);
            }
        }        
    }    
}

//curl -i 'https://ord.servers.api.rackspacecloud.com/v2/863421/servers/cf5c8b43-e2a5-4e57-bf65-0997ced9d548' -X DELETE -H "Accept: application/json" -H "User-Agent: python-novaclient" -H "X-Auth-Project-Id: opencloudlab" -H "X-Auth-Token: {SHA1}ecfbd5a1d193d288878dd7b4d727c6ab0c9f8308"
int delete(struct auth_info Ainfo, char* serverid)
{
    CURL *curl;
    CURLcode res;
    int success = 0;
    struct string s;
    init_string(&s);
    curl = curl_easy_init();
    if(curl) {
        char tmp[70] = "X-Auth-Token: ";
        strcat(tmp, Ainfo.tokenid);
        
        char url[200];
        strcpy(url, "https://");
        strcat(url, Ainfo.region);
        strcat(url, ".servers.api.rackspacecloud.com/v2/");
        strcat(url, Ainfo.tenantid);
        strcat(url, "/servers/");
        strcat(url, serverid);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        /* example.com is redirected, so we tell libcurl to follow redirection */ 
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "User-Agent: python-novaclient");
        headers = curl_slist_append(headers, "X-Auth-Project-Id: opencloudlab");
        headers = curl_slist_append(headers, tmp);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

         
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            success = 0;
        }
        else success = 1;
        //printf("%s\n", s.ptr);        // response

        /* always cleanup */ 
        curl_easy_cleanup(curl);
  }
  
  return success;
}

// curl -i 'https://ord.servers.api.rackspacecloud.com/v2/863421/servers/cf5c8b43-e2a5-4e57-bf65-0997ced9d548' -X GET -H "Accept: application/json" -H "User-Agent: python-novaclient" -H "X-Auth-Project-Id: opencloudlab" -H "X-Auth-Token: {SHA1}ecfbd5a1d193d288878dd7b4d727c6ab0c9f8308"
int isServerExist(struct auth_info Ainfo, char* serverid)
{
    CURL *curl;
    CURLcode res;
    int success = 0;
    struct string s;
    init_string(&s);
    curl = curl_easy_init();
    if(curl) {
        char tmp[70] = "X-Auth-Token: ";
        strcat(tmp, Ainfo.tokenid);
        
        char url[200];
        strcpy(url, "https://");
        strcat(url, Ainfo.region);
        strcat(url, ".servers.api.rackspacecloud.com/v2/");
        strcat(url, Ainfo.tenantid);
        strcat(url, "/servers/");
        strcat(url, serverid);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        /* example.com is redirected, so we tell libcurl to follow redirection */ 
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "User-Agent: python-novaclient");
        headers = curl_slist_append(headers, "X-Auth-Project-Id: opencloudlab");
        headers = curl_slist_append(headers, tmp);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

         
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            success = 0;
        }
        else success = 1;
        
        int r, i;
        jsmn_parser p;
        jsmntok_t tokens[s.len];
        jsmn_init(&p);
        r = jsmn_parse(&p, s.ptr, s.len, tokens, s.len);
        for (i = 1; tokens[i].end < tokens[0].end && tokens[i].end > 0; i++) {
            if (tokens[i].type == JSMN_STRING || tokens[i].type == JSMN_PRIMITIVE) {
                //printf("%.*s--\n", tokens[i].end - tokens[i].start, s.ptr + tokens[i].start);
                if(strncmp(s.ptr + tokens[i].start, "code", tokens[i].end - tokens[i].start ) == 0)    // tokenid
                {
                    i++;
                    if(strncmp(s.ptr + tokens[i].start, "404", tokens[i].end - tokens[i].start ) == 0)  success = 0;
                    else                                                                                success = 1;
                    break;
                }
            }
        }

        /* always cleanup */ 
        curl_easy_cleanup(curl);
  }
  return success;

}
//=============================== End functions ================================    


int main(int argc, char* argv[])
{  
    
    struct auth_info Ainfo;
    char* usage = "Usage: opengahp [token, create [name], list, delete [id]]\n";
    if(argc <= 1) {
        printf("%s", usage);
        return 0;
    }
    
    if(!strcmp(argv[1], "token")) {
        Ainfo = getAuthInfo();
        if(!Ainfo.filled)     printf("Cannot get token!\n");
        else                  printf("Token = %s\n", Ainfo.tokenid);
        return 0;
    }
    
    else if(!strcmp(argv[1], "create")) {
        if(argc < 3) {
            printf("%s", usage);
            return 0;
        }
        printf("Gettting token...\n");
        Ainfo = getAuthInfo();
        if(!Ainfo.filled) {
            printf("Cannot get token!\n");
            return 0;
        }
        printf("Token = %s\n", Ainfo.tokenid);  
        int res = createServer(Ainfo, argv[2]);
        if(!res) {
            printf("Cannot create server!\n");
            return 0;
        }        
        printf("Server has been created successfully!\n");
        return 0;
    }
    
    else if(!strcmp(argv[1], "list")) {
        Ainfo = getAuthInfo();
        struct string s;
        if(!Ainfo.filled) {
            printf("Cannot get token!\n");
            return 0;
        }
        s = list(Ainfo);
        if(s.len <= 0) {
            printf("Cannot get list!\n");
            return 0;
        }
        printList(s);
    }
    
    else if(!strcmp(argv[1], "delete")) {
        if(argc < 3) {
            printf("%s", usage);
            return 0;
        }        
        printf("Gettting token...\n");
        Ainfo = getAuthInfo();
        if(!Ainfo.filled) {
            printf("Cannot get token!\n");
            return 0;
        }
        printf("Token = %s\n", Ainfo.tokenid);
        if(!isServerExist(Ainfo, argv[2])) {
            printf("Instance could not be found\n");
            return 0;
        }
        int res = delete(Ainfo, argv[2]);
        if(!res) {
            printf("Cannot delete server!\n");
            return 0;
        }        
        printf("Server has been deleted successfully!\n");
        return 0;
    }
    
    else 
        printf("%s", usage);
    return 0;
}

// compile command: gcc simple.c -o simple.a -lcurl -L. -ljsmn


