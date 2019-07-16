
#include "types.h"

extern char *strcpy(char *s, const char *ct);
extern char *strncpy(char *s, const char *ct, LONG n);
extern char *strcat(char *s, const char *ct);
extern char *strncat(char *s, const char *ct, LONG n);
extern long strcmp(const char *cs, const char *ct);
extern long stricmp(const char *cs, const char *ct);
extern long strncmp(const char *cs, const char *ct, LONG n);
extern long strnicmp(const char *cs, const char *ct, LONG n);
extern long strcspn(char *s1, char *s2);
extern char *strchr(char *s, LONG c);
extern char *strrchr(char *s, LONG c);

extern long strlen(char *cs);
extern char *strupr(char *string);
extern char *strlwr(char *string);

extern LONG atol(char *string);
extern LONG atoi(char *string);
extern LONG atox(char *string);
