
/**
 * return the socket
 */
int create_connection(char* ip_address, unsigned short port);

/**
 * Replace any '/' with '_'
 */
 int escape_nid(char* nid, long nid_length);
 
  /**
 * Replace any '_' with '/'
 */
 int reverse_escape_nid(char* nid, long nid_length);
 
/**
  * @param byte_array must be an array of four unsigned char
  */
 unsigned int from_byte_array_to_number(unsigned char* byte_array);
