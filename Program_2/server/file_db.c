/* Name: Abhimanyu Choudhary
Student_Id:00001537566*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "file_db.h"

int getSubscriberInfo(unsigned int number, SubscriberInfo_t *info)
{
    FILE *ptr = NULL;
    char buffer[256];

    ptr = fopen("Verification_Database.txt", "r");
    if (ptr == NULL) {
        printf("Unable to open file");
        return -1;
    }

    char* _t = NULL;
    while ((fgets(buffer, 256, ptr)) != NULL) {
        _t = strtok(buffer, ";");
        if (_t != NULL && number == strtoul(_t,NULL,10)) {
            info->number = strtoul(_t,NULL,10);
            _t = strtok(NULL, ";");
            info->technology = strtoul(_t,NULL,10);
            _t = strtok(NULL, ";");
            info->payment_stat = strtoul(_t,NULL,10);
            fclose(ptr);
            return 0;
        }
    }

    fclose(ptr);
    return -1;
}
