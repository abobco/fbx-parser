/* 
    By Austin Bobco 9/22/2020

    Written using info from this article
    https://code.blender.org/2013/08/fbx-binary-file-format-specification/

    WIP
*/

// gcc linked_list.c fbxparse.c -o fbxparse -g -Lzlib/lib -Izlib/include -lz 

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>

#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <sys/stat.h>

#include "zlib.h"
#include "linked_list.h"

#define CHUNK 16384

#define pint(val) printf("%s = %d\n", #val, val)
#define pflt(val) printf("%s = %f\n", #val, val)

static const char *RelevantNodes[] = {
    "Geometry",
    "Vertices",
    "PolygonVertexIndex",
    "Edges",
    "Normals",
    "UV",
    "UVIndex",
    NULL
};

typedef struct ObjectAttribute {
    ListNode *attributes;
    char *name;
} ObjectAttribute;

bool string_in_list(const char* str, const char **str_list) {
    int i =0;
    while ( RelevantNodes[i] != NULL )
        if ( strcmp(str, str_list[i++]) == 0 )
            return true;
    return false;
}

int inflate_array(char *buf, int idx, unsigned char *out, int defl_size) {
    unsigned char in[defl_size];

    /* allocate inflate state */
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    inflateInit(&strm);
    strm.avail_in = defl_size;
    strm.next_in = &(buf[idx]);

    strm.avail_out = CHUNK;
    strm.next_out = out;
    return inflate(&strm, Z_NO_FLUSH);
}

void visit_prop(char *buf, int *start_idx, ListNode **attr_list_head) {
    int idx = *start_idx;
    char prop_type = buf[idx++];
    // printf("\n\tProp type: %c\n", prop_type);

    switch (prop_type) {
    // arrays
    case 'd':   // 8 byte IEEE 754 floats
    {
        uint32_t *ibuf = (uint32_t*) &buf[idx];
        uint32_t array_len = *ibuf++;
        uint32_t encoding = *ibuf++;
        uint32_t deflated_size = *ibuf++;
        idx += sizeof(uint32_t)*3;

        if ( encoding == 1 ) {
            // zipped array
            const unsigned inflated_size = array_len*sizeof(double);
            unsigned char out[inflated_size];
            int result = inflate_array(buf, idx, out, deflated_size);

            list_append(attr_list_head, &out, inflated_size, prop_type);
        } else {
            list_append(attr_list_head, &buf[idx], deflated_size, prop_type);
        }
        idx += deflated_size;

    }
    break;

    case 'i':   // 4 byte signed ints
    {
        uint32_t *ibuf = (uint32_t*) &buf[idx];
        uint32_t array_len = *ibuf++;
        uint32_t encoding = *ibuf++;
        uint32_t deflated_size = *ibuf++;
        idx += sizeof(uint32_t)*3;
        if ( encoding == 1) {
            // zipped array
            const unsigned inflated_size = array_len*sizeof(double);
            unsigned char out[inflated_size];
            int result = inflate_array(buf, idx, out, deflated_size);
            
            list_append(attr_list_head, &out, inflated_size, prop_type);
        } else {
            list_append(attr_list_head, ibuf, sizeof(int32_t)*array_len, prop_type);
        }
        idx += deflated_size;
    }
    break;

    // string
    case 'S':
    {
        uint32_t slen = *((uint32_t*) &buf[idx]);
        idx += sizeof(slen);
        char *sval = malloc(sizeof(char) * (slen+1));
        memcpy( sval, &buf[idx], slen);
        sval[slen] = '\0';
        printf("\t%s\n", sval);
        free(sval);
        idx += slen;
    }
    break;

    // scalars
    case 'C':   // 1 bit bool, the LSB of a 1 byte value
    {
        bool val = buf[idx];
        printf("\t%d\n", val);   
        idx++;
    }
    break;

    case 'Y':   // 2 byte signed int
    {
        int16_t val = *((int16_t*) &buf[idx]);
        printf("\t%d\n", val);  
        idx += sizeof(val);
    }
    break;

    case 'I':   // 4 byte signed int
    {   
        int32_t val = *((int32_t*) &buf[idx]);
        printf("\t%d\n", val);  
        idx += sizeof(val);
    }
    break;

    case 'L':   // 8 byte signed int
    {
        long long L_type = *((long long*) &buf[idx]);
        printf("\t%d\n", L_type);  
        idx += sizeof(L_type);
    }
    break;

    case 'F':   // 4 byte IEEE 754 float
    {
        float val = *((float*) &buf[idx]);
        printf("\t%f\n", val);   
        idx += sizeof(val);
    }
    break;
        
    case 'D':   // 8 byte IEEE 754 float
    {
        double val = *((double*) &buf[idx]);
        printf("\t%f\n", val);   
        idx += sizeof(val);
    }
    break;

    // raw binary
    case 'R':
    default:
        exit(-1);
    break;  
    }
    *start_idx = idx;
}

int visit_node(char *buf, int start_idx, ListNode **attr_list) {
    uint32_t end_offset;
    uint32_t num_props;
    uint32_t prop_list_len;
    uint8_t  name_len;

    end_offset =     *((uint32_t*) &buf[start_idx]);
    if ( end_offset == 0 )
        return 0;

    num_props  =     *((uint32_t*) &buf[start_idx+4]);
    prop_list_len =  *((uint32_t*) &buf[start_idx+8]);
    name_len  =      *((uint8_t*)  &buf[start_idx+12]);

    char *name = malloc(sizeof(char) * (name_len+1));
    memcpy( name,           &buf[start_idx+13], name_len);
    name[name_len] = '\0';
      
    int curr_pos = start_idx + 13 + name_len;
    if ( string_in_list(name, RelevantNodes) ) {
        printf("\nNode '%s': bytes %d - %d\n", name, start_idx, end_offset);
        printf("\tNumProperties: %d\n", num_props);
        printf("\tPropertyListLen: %d\n", prop_list_len);
        
        ObjectAttribute new_attr;
        new_attr.attributes = NULL;
        new_attr.name = malloc(name_len+1);
        strcpy(new_attr.name, name);

        for ( int i = 0; i < num_props; i++ ) {
            visit_prop(buf, &curr_pos, &new_attr.attributes);
        }
        list_append(attr_list, &new_attr, sizeof(new_attr), 'O');
        curr_pos -= prop_list_len;
    }

    free(name);
    curr_pos += prop_list_len;
    bool needs_offset = false;
    if ( curr_pos == end_offset )
        return curr_pos;

    while ( curr_pos+13 <  end_offset ) {
        curr_pos = visit_node(buf, curr_pos, attr_list);
    }
    // pint(curr_pos);
    // pint(end_offset);
    curr_pos+=13;
    return curr_pos;
}

int main(int argc, char** argv) {
    struct stat finfo;
    char filename[1024]; 
    if ( argc > 1 ) {
        strcpy(filename, argv[1]);
    } else {
        printf("usage: fbxparse filename.fbx\n");
        exit(0);
    }
    stat(filename, &finfo);
    printf("'%s' size: %d bytes\n", filename, finfo.st_size); 

    int fd = open(filename, O_RDONLY);
    if ( fd != -1 ) {
        char *fbuf = malloc(sizeof(char) * finfo.st_size);
        read(fd, fbuf, finfo.st_size);
        close(fd);
        
        // read header
        char header[20];
        memcpy( &header, &fbuf[0], 20);
        printf("%s\n", header);

        unsigned int version;
        memcpy( &version, &fbuf[23], 4);
        printf("Version: %d\n", version);

        // read node record
        ListNode *attribute_list = NULL;
        int pos = 27;
        while( pos < finfo.st_size-300 ) {
            pos = visit_node(fbuf, pos, &attribute_list);
        }

        // print loaded data
        ListNode *iter = attribute_list;
        while ( iter != NULL) {
            assert( iter->type == 'O');
            ObjectAttribute *attr = (ObjectAttribute*)iter->data;
            printf("\n\n%s:", attr->name);
            ListNode *attr_iter = attr->attributes;
            while ( attr_iter != NULL ) {
                switch (attr_iter->type) {
                case 'd':
                {
                    double *view = (double*) attr_iter->data;
                    for ( int i = 0; i < attr_iter->size/sizeof(double); i++ ) {
                        if ( i % 3 == 0)
                            printf("\n\t");
                        printf("%f ", view[i]);
                    }
                }
                break;
                
                case 'i':
                {
                    int32_t *view = (int32_t*) attr_iter->data;
                    for ( int i = 0; i < attr_iter->size/sizeof(int32_t); i++ ) {
                        if ( i % 3 == 0)
                            printf("\n\t");
                        printf("%d ", view[i]);
                    }
                }
                break;
                }
                attr_iter = attr_iter->next;
            }   
            iter=iter->next;
        }
        free(fbuf);
    }

}