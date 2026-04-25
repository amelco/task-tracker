# implement a string builder

# STATUS: CLOSED
# PRIORITY: 80

API:
StringBuilder *sb_append(char *dest, const char *text); 
 - the va-args are all other strings to be appended
 - allocates in the heap
void sb_free(StringBuilder *sb);

