/*
 * Standalone temperature logger
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "temper.h"
#include "pcsensor.h"

#define SEQFILE "/tmp/aprsTemperSequence"

/* Calibration adjustments */
/* See http://www.pitt-pladdy.com/blog/_20110824-191017_0100_TEMPer_under_Linux_perl_with_Cacti/ */
static float scale = 1.0287;
static float offset = -0.85;

int main(int argc, char **argv)
{
    unsigned char optChar;
    BOOL label = FALSE;
    BOOL unit = FALSE;
    BOOL eq = FALSE;
    BOOL telem = FALSE;
    BOOL status = FALSE;
    char * callSign = NULL;

    while((optChar = getopt (argc, argv, "luetsc:")) != 255)
    {
        switch(optChar)
        {
        case 'l':
            label = TRUE;
            break;
        case 'u' :
            unit = TRUE;
            break;
        case 'e':
            eq = TRUE;
            break;
        case 't':
            telem = TRUE;
            break;
        case 's':
            status = TRUE;
            break;
        case 'c':
            callSign = optarg;
            break;
        default:
            return 1;
        }
    }

    if(callSign == NULL)//no callsign specified, exit
        return 1;

    if(label)
        print_labels(callSign);
    if(unit)
        print_units(callSign);
    if(eq)
        print_equation(callSign);
    if(telem)
        print_telemetry();
    if(status)
        print_status();

    return 0;
}

void print_labels(char * callSign)
{
    printf(":%-9s:PARM.PA Temp\n", callSign);
    fflush(stdout);
}

void print_units(char * callSign)
{
    printf(":%-9s:UNIT.deg.C\n", callSign);
    fflush(stdout);
}

void print_equation(char * callSign)
{
    printf(":%-9s:EQNS.0,0.16016,-40,0,0,0,0,0,0,0,0,0,0,0,0\n", callSign);
    fflush(stdout);
}

void print_status()
{
    float temp;
    if(!read_temp(&temp))
    {
        printf(">PA Temperature : %.1f°C\n", temp);
        fflush(stdout);
    }
}

void print_telemetry(void)
{
    int seq;
    float temp;

    if(!read_temp(&temp))
    {
        seq = get_telemetry_sequence();

        temp += 40;
        temp /= 0.16016;

        printf("T#%03d,%.0f,000,000,000,000,00000000\n", seq, temp);//since aprs specs says datas is unsigned ad 40 to it, the 40 will substracted by listeners using EQNS
        fflush(stdout);
    }
}

int get_telemetry_sequence(void)
{
    int seq = 0;
    FILE * fileds = fopen(SEQFILE, "rb");
    if(fileds)
    {
        fread(&seq, sizeof(int), 1, fileds);
        fclose(fileds);
    }

    seq++;

    if(seq > 999)
        seq = 1;

    fileds = fopen(SEQFILE, "wb");
    if(fileds)
    {
        fwrite(&seq, sizeof(int), 1, fileds);
        fclose(fileds);
    }

    return seq;
}

int read_temp(float * temp)
{
    int passes = 0;
    float tempc = 0.0000;
    do
    {
        usb_dev_handle* lvr_winusb = pcsensor_open();

        if (!lvr_winusb)
        {
            /* Open fails sometime, sleep and try again */
            sleep(3);
        }
        else
        {

            tempc = pcsensor_get_temperature(lvr_winusb);
            pcsensor_close(lvr_winusb);
        }
        ++passes;
    }
    /* Read fails silently with a 0.0 return, so repeat until not zero
       or until we have read the same zero value 3 times (just in case
       temp is really dead on zero */
    while ((tempc > -0.0001 && tempc < 0.0001) || passes >= 4);

    if (!((tempc > -0.0001 && tempc < 0.0001) || passes >= 4))
    {
        /* Apply calibrations */
        tempc = (tempc * scale) + offset;
        *temp = tempc;

        return 0;
    }
    else
    {
        return 1;
    }
}
