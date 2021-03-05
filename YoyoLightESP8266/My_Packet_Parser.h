struct udp_msg
{
    uint8_t header;
    uint8_t content_start_byte;
};

typedef void (*callback_func)(udp_msg *); //

const uint8_t NUM_HEADERS = 10;
void (*function_map[NUM_HEADERS])(udp_msg *); //associates function ptr to a header key

//params:   void function with one param of udp_msg
//          uint8_t header to associate with it
bool Assign_Function_To_Header(callback_func func_ptr, uint8_t header)
{
    if (header >= NUM_HEADERS)
    {
        return false;
    }
    function_map[header] = func_ptr;
    return true;
}

bool Call_Header_Function(uint8_t *raw_packet_ptr)
{
    udp_msg *message = (udp_msg *)(raw_packet_ptr); //cast the msg into a udp_message
    if (message->header < NUM_HEADERS)
    {
        function_map[message->header](message); //call the associated function for the header
        return true;
    }
    return false;
}
