#include <stdio.h>

struct person {
        char *firstName;
        char *lastName;
        char *birthYear;
};

void person_printFirstName(struct person *this) {
        printf("The first name is: %s\n", this->firstName);
}
