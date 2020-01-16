#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//===========Definations=============
#define MAX 2048

//==============Globals============
struct tree
{
    struct tree *left;
    struct tree *right;
    unsigned int ch;
    unsigned int freq;
    unsigned int leaf;
};
//FILE *fc;
typedef struct tree Tree;
char *codeTable[MAX] = {0};

//============Function Prototypes=============
void compress(char *, char *);
void decompress(char *, char *);
void get_stats(unsigned char *, unsigned int *, unsigned int);
Tree *makeTree(FILE *, unsigned int *, FILE *);
Tree *makeTreeDcom(unsigned int *);
void sortForest(Tree **forest, int);
void removeTree(Tree **forest, int, int);
void genCodes(Tree *, char *, int);
//================Main Function======================
int main(int argc, char **argv)
{
    FILE *fr, *fw;
    char *fWname = NULL, *fRname = NULL;
    int optIndx = 0;
    int decomp = 0;
    // printf("Starting\n");
    while ((optIndx = getopt(argc, argv, "d")) != -1)
    {

        switch (optIndx)
        {
        case 'd':
            if (argc > 3)
            {
                fRname = argv[2];
                fWname = argv[3];
                decompress(fRname, fWname);
                decomp = 1;
            }

            break;
        case '?':
            printf("unknown option: %c\n", optopt);
            break;
        }
    }

    if (!decomp)
    {
        if (argc == 1)
        {

            fprintf(stderr, "ERROR: File Names Not Provided\n");
            fprintf(stderr, "Press Enter to Terminate Program!!\n");
            getchar();
            exit(1);
        }
        else
        {

            //printf("Arg count: %d arg1: %s  arg2: %s\n", argc, argv[1], argv[2]);
            fRname = argv[1];
            fWname = argv[2];
            compress(fRname, fWname);
        }
    }

    //    printf("Done!!!\n");
    //    getchar();
    return 0;
}

/*=============================Compress Function===================
Input: input and output file names
Output: Encrypted file with the name give as second argument
Process: Get stats of the file
         Make a huffman tree
         Traverse the generated tree to create codes for each character in the input file
         Read the input file again and put respective codes in the output file

==================================================================*/
void compress(char *fIn, char *fOut)
{
    //printf("Starting Compression\n");
    FILE *fr, *fw, *feq;

    long fIn_size;
    unsigned int fSize;
    unsigned char *fBuff;
    Tree *fTree = NULL;
    // unsigned int *freqTable = (unsigned int *)calloc(MAX, sizeof(int));
    unsigned int freqTable[MAX] = {0};

    fpos_t posN, posP, posN1;
    char codes[MAX];
    char *code;
    unsigned int chRead = 0;
    unsigned int chCode, count = 0;
    unsigned char chWrite = 0;
    unsigned int bitcount = 0;
    unsigned char padding = 0;

    //==========================================

    fr = fopen(fIn, "r");
    if (fr == NULL)
    {
        fprintf(stderr, "Error opening %s file \n", fIn);
        exit(1);
    }

    fseek(fr, 0, SEEK_END);
    fIn_size = ftell(fr);

    if (fIn_size > 0)
    {
        fBuff = malloc(fIn_size * sizeof(unsigned char));
    }
    else
    {
        fprintf(stderr, "%s File is Empty\n", fIn);
        exit(1);
    }
    fSize = (unsigned int)fIn_size;
    //printf("fSize in bytes: %d\n", fSize);

    fw = fopen(fOut, "wb");
    if (fw == NULL)
    {
        fprintf(stderr, "Error opening %s file \n", fOut);
        exit(1);
    }

    fwrite(&fSize, sizeof(int), 1, fw); // total no of chars found in the file

    rewind(fr);
    fread(fBuff, sizeof(char), fIn_size, fr);
    fclose(fr);

    get_stats(fBuff, freqTable, fSize);

    feq = fopen("freq.txt", "w");
    if (feq == NULL)
        perror("Error opening file");

    fTree = makeTree(feq, freqTable, fw);
    fclose(feq);

    fgetpos(fw, &posN);
    fwrite(&padding, sizeof(char), 1, fw);

    genCodes(fTree, codes, 0);
    // printf("Codes generated\n");
    for (count = 0; count < fIn_size; count++)
    {

        chRead = fBuff[count];
        code = codeTable[chRead];

        for (unsigned int i = 0; i < strlen((const char *)code); i++)
        {

            chCode = code[i];
            if (chCode == '1')
            {

                chWrite <<= 1;
                chWrite |= 1;
                bitcount++;
            }
            else
            {
                bitcount++;
                chWrite <<= 1;
            }
            if (bitcount == 8)
            {
                fputc(chWrite, fw);
                bitcount = 0;
                chWrite = 0;
            }
        }
    }
    if (bitcount != 0)
    {
        chWrite <<= (8 - bitcount);
        padding = 8 - bitcount;
        fputc(chWrite, fw);
    }
    fgetpos(fw, &posP);
    fsetpos(fw, &posN);
    fwrite(&padding, sizeof(char), 1, fw);
    fsetpos(fw, &posP);
    // printf("total padding bits written: %d pos of bits: %d \n", padding, posN);

    //===========End Section: House Keeping=====================
    fclose(fw);
    free(fBuff);
    //free(freqTable);
    //printf("Compression End\n");
}
//=========================================================================
void get_stats(unsigned char *fBuff, unsigned int *freqTable, unsigned int fIn_size)
{
    //printf("Getting Stats\n");
    unsigned int ch = 0;
    for (unsigned int i = 0; i < fIn_size; i++)
    {
        ch = fBuff[i];
        freqTable[ch]++;
    }
}
//===========************************====================
Tree *makeTree(FILE *fp, unsigned int *freqTable, FILE *fo)
{
    // printf("Creating Tree!!! \n");
    fpos_t posN, posP;
    fgetpos(fo, &posN);
    int dummy = 0;

    fwrite(&dummy, sizeof(int), 1, fo);

    Tree *forest[MAX];
    unsigned int charFound = 0;
    int trees = 0, min1, min2;
    fprintf(fp, "No: \tASCII	\tDecimal		\tfreq   \n");
    int tempCount = 0;

    //================================================
    for (unsigned int i = 0; i < MAX; i++)
    {
        if (freqTable[i] > 0)
        {
            forest[charFound] = (Tree *)malloc(sizeof(Tree));
            forest[charFound]->left = 0;
            forest[charFound]->right = 0;
            forest[charFound]->ch = i;
            forest[charFound]->freq = freqTable[i];
            forest[charFound]->leaf = 1;
            fprintf(fp, "%03d \t%c	\t%d		 \t%d\n", charFound + 1, forest[charFound]->ch, forest[charFound]->ch, forest[charFound]->freq);

            fwrite(&i, sizeof(int), 1, fo);
            fwrite(&freqTable[i], sizeof(int), 1, fo);

            charFound++;
        }
    }
    if (charFound == 0)
    {
        perror("File is Empty!!!\n");
        // getchar();
        exit(1);
    }
    fgetpos(fo, &posP);
    fsetpos(fo, &posN);
    fwrite(&charFound, sizeof(int), 1, fo);
    fsetpos(fo, &posP);

    trees = charFound;
    Tree *t1, *t2, *t;
    while (trees > 1)
    {
        //printf("********Sorting starting*********\n");
        sortForest(forest, trees);
        t1 = forest[0];
        t2 = forest[1];
        t = (Tree *)malloc(sizeof(Tree));

        t->freq = t1->freq + t2->freq;
        t->ch = 0;
        t->leaf = 0;
        t->left = t1;
        t->right = t2;
        forest[0] = t;

        if (trees > 1)
            removeTree(forest, 1, trees);
        trees--;
    }
    return forest[0];
}

//==========************************================
void sortForest(Tree **forest, int count)
{
    int gap, i, j;
    Tree *temp = NULL;
    for (gap = count / 2; gap > 0; gap /= 2)
        for (i = gap; i < count; i++)
            for (j = i - gap; j >= 0 && (forest[j]->freq > forest[j + gap]->freq); j -= gap)
            {
                temp = forest[j];
                forest[j] = forest[j + gap];
                forest[j + gap] = temp;
            }
}
//=============================================
void removeTree(Tree **forest, int ind, int total)
{
    for (int i = ind; i < total; i++)
    {
        {
            forest[i] = forest[i + 1];
        }
    }
}
//===============================================================================
char *stCode;
void genCodes(Tree *tree, char code[], int ind)
{
    if (tree->left)
    {
        code[ind] = '0';
        genCodes(tree->left, code, (ind + 1));
    }

    if (tree->right)
    {
        code[ind] = '1';
        genCodes(tree->right, code, (ind + 1));
    }

    if (tree->leaf)
    {
        code[ind] = '\0';
        stCode = (char *)malloc(strlen((char *)code) * sizeof(char));
        strcpy(stCode, (char *)code);
        codeTable[tree->ch] = stCode;
    }
}
//================================================
void decompress(char *fIn, char *fOut)
{

    //printf("********Starting Decompression**********\n");

    FILE *fd = fopen(fOut, "w");
    if (fd == NULL)
    {
        fprintf(stderr, "Error opening %s file \n", fOut);
        exit(1);
    }

    FILE *fr = fopen(fIn, "rb");
    if (fr == NULL)
    {
        fprintf(stderr, "Error opening %s file \n", fIn);
        exit(1);
    }
    long fIn_size = 0;
  fseek(fr, 0, SEEK_END);
    fIn_size = ftell(fr);

    if (fIn_size == 0)
    {
       
        fprintf(stderr, "%s File is Empty\n", fIn);
        exit(1);
    }
	rewind(fr);
    //=====================================================
    Tree *rTree, *current;
    unsigned int fsize = 0;
    unsigned int ch = 0, freq = 0, count = 0;
    unsigned int bytesPrinted = 0;
    unsigned int chFound = 0, bit, loopCount = 8, diff = 0;
    unsigned char padding;
    unsigned char mask = (1 << 7);

    //=========================================================

    fread(&fsize, sizeof(int), 1, fr);
    fread(&chFound, sizeof(int), 1, fr);
    if (chFound == 1)
    {

        // printf("Only One Char\n");
        ch = fgetc(fr);
        fputc(ch, fd);
        fclose(fd);
        fclose(fr);
        //printf("Decompression Completed\n");
        return;
    }

    //=====================================
    else
    {
        unsigned int *freqTable = (int *)calloc(MAX, sizeof(int));

        for (int i = 0; i < chFound; i++)
        {
            fread(&ch, sizeof(int), 1, fr);
            fread(&freq, sizeof(int), 1, fr);
            freqTable[ch] = freq;
        }

        fread(&padding, sizeof(char), 1, fr);

        rTree = makeTreeDcom(freqTable);
        current = rTree;

        for (bytesPrinted = 0; bytesPrinted < fsize; count++)
        {
            ch = fgetc(fr);
            // c = ch;

            diff = fsize - bytesPrinted;
            if (diff == 1)
            {
                loopCount = 8 - padding;
            }
            else
            {
                loopCount = 8;
            }

            for (int i = 0; i < loopCount; i++)
            {
                bit = (ch & mask);
                ch <<= 1;
                if (bit == 0)
                {
                    current = current->left;
                    if (current->leaf)
                    {
                        fputc(current->ch, fd);
                        bytesPrinted++;
                        if (bytesPrinted == fsize)
                        {
                            break;
                        }
                        current = rTree;
                    }
                }
                else
                {
                    current = current->right;
                    if (current->leaf)
                    {
                        fputc(current->ch, fd);
                        bytesPrinted++;
                        if (bytesPrinted == fsize)
                        {
                            break;
                        }
                        current = rTree;
                    }
                }
            }
        }

        fclose(fd);
        fclose(fr);
        //printf("Decompression Completed\n");
        free(freqTable);
    }
}
//===============================================
Tree *makeTreeDcom(unsigned int *freqTable)
{
    Tree *forest[MAX];
    unsigned int charFound = 0;
    unsigned int trees = 0, min1, min2;
    for (unsigned int i = 0; i < MAX; i++)
    {
        if (freqTable[i] > 0)
        {
            forest[charFound] = (Tree *)malloc(sizeof(Tree));
            forest[charFound]->left = 0;
            forest[charFound]->right = 0;
            forest[charFound]->ch = i;
            forest[charFound]->leaf = 1;
            forest[charFound]->freq = freqTable[i];
            charFound++;
        }
    }

    trees = charFound;
    Tree *t1, *t2, *t;
    while (trees > 1)
    {
        sortForest(forest, trees);

        t1 = forest[0];
        t2 = forest[1];
        t = (Tree *)malloc(sizeof(Tree));

        t->freq = t1->freq + t2->freq;
        t->ch = 0;
        t->leaf = 0;
        t->left = t1;
        t->right = t2;
        forest[0] = t;
        if (trees > 1)
            removeTree(forest, 1, trees);
        trees--;
    }
    return forest[0];
}
