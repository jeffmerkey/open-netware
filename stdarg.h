



typedef char *va_list[1];

#define va_start(ap, parm)  ((ap)[0] = (char *) &parm \
	 + ((sizeof(parm) + sizeof(int) - 1) & ~(sizeof(int) - 1)), (void) 0)

#define va_arg(ap, type)  ((ap)[0] += \
	 ((sizeof(type) + sizeof(int) - 1) & ~(sizeof(int) - 1)), \
	     (*(type *) ((ap)[0] \
	 - ((sizeof(type) + sizeof(int) - 1) & ~(sizeof(int) - 1)) )))

#define va_end(ap)   ((ap)[0] = 0, (void) 0)
