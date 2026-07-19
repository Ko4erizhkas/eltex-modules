
typedef struct Person
{
    char* firstName;
    char* lastName;
    char* surname;
    char* emailAddress;
    char* phoneNumber;
    char* media;
} Person;
void addPerson();

struct PhoneBook
{
    Person data;
    struct PhoneBook* next;
};

void editPerson(Person p);
void deletePerson(Person p);