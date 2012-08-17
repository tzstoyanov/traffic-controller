#ifndef TST_H
#define TST_H

    // open the output stream
    void TSTOpen( const char * pStr );

    // close the output stream
    void TSTClose( void );

    // write to output stream
    void TSTOutStr( const char *pStr );

    // write 16bit hex value
    void TSTOut16H(U16 Value);

    // write 32bit hex value
    void TSTOut32H(U32 Value);

    void TSTOut8H(u08 U);

#endif
