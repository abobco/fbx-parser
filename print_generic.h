// requires C11 compiler
#define printf_dec_format(x) _Generic((x), \
    char: "%c", \
    signed char: "%hhd", \
    unsigned char: "%hhu", \
    signed short: "%hd", \
    unsigned short: "%hu", \
    signed int: "%d", \
    unsigned int: "%u", \
    long int: "%ld", \
    unsigned long int: "%lu", \
    long long int: "%lld", \
    unsigned long long int: "%llu", \
    float: "%f", \
    double: "%f", \
    long double: "%Lf", \
    char *: "%s", \
    void *: "%p")

#define print(x) printf("%s = ", #x), printf(printf_dec_format(x), x)  
#define println(x) printf("%s = ", #x), printf(printf_dec_format(x), x), printf("\n");

#define fprint(f, x) fprintf(f, "%s = ", #x), fprintf(f, printf_dec_format(x), x)  
#define fprintln(f, x) fprintf(f, "%s = ", #x), fprintf(f, printf_dec_format(x), x), fprintf(f, "\n");

#define print_vec(a) printf("%s = (%f, %f, %f)\n", #a, a.x, a.y, a.z );