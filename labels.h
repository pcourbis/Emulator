struct label
{
    unsigned long int address;
    char *txt;

    struct label *next;
};

extern void AddLabel();
extern void CloseLabels();
extern struct label *FindLabel();
extern int IsIn();
extern void LoadLabels();
extern char *NextSep();
extern void OutInTable();
