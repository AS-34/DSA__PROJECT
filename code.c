#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define CONSOLE_INPUT_LENGTH 64
#define MAX_DIST_ALLOWED 3
#define SUBSTITUTION_COST 1

const char hashedDictFile[] = "dictionary.bin";

typedef struct {

    char** table;
    int size;

} HashTable_t;
HashTable_t* hashTableLoadFromText(FILE* fp);
HashTable_t* hashTableLoadFromBinary(FILE* fp);
void hashTableFree(HashTable_t* hm);
void hashTableSaveAsBinary(HashTable_t* hashTable, const char* filePath);
int hashTableFindKey(HashTable_t* hashTable, char* key, int keyLen);
int hashTableCalculateOptimalSize(int numberOfElements);
int hashTableGetHash(HashTable_t* hashTable, char* key, int keyLen);
int getLineCount(FILE* fp);
int getMin(int x, int y, int z);
bool isPrime(int x);
bool hasInvalidChars(char* string);
void toLower(char string[], int len);
short findMostSimilarWord(HashTable_t* hashTable, char* key, char** wordFound);
short getEditDistance(char* str1, char* str2);
char* getEditPath(char* str1, char* str2);
short** getEditDistanceMatrix(char* str1, short len1, char* str2, short len2);
int main() {
system ("color 5f");
    FILE* fDictHashed = fopen(hashedDictFile, "rb");
    HashTable_t* hashedDict;
    if(fDictHashed == NULL) {
        printf("Binary dictionary file not found.\n"
               "Please enter the ASCII formatted dictionary file: ");
        char input[CONSOLE_INPUT_LENGTH];
        scanf("%s", input);
        FILE* fTextDict;
        while( ( fTextDict = fopen(input,"r") ) == NULL ) {
            printf("ERROR: Could not read file: %s\n"
                   "Please enter the ASCII formatted dictionary file: ", input);
            scanf("%s", input);
        }
        hashedDict = hashTableLoadFromText(fTextDict);
        fclose(fTextDict);
        hashTableSaveAsBinary(hashedDict, hashedDictFile);
    } else {
        hashedDict = hashTableLoadFromBinary(fDictHashed);
        fclose(fDictHashed);
    }
    printf("\t\t\t\tDictionary has been loaded into the memory.\n");
    printf("\t\t\t\tEnter -1 to exit the program.\n");
    bool cmdQuit = false;
    while(!cmdQuit) {
        printf("\t\t\t\tEnter a word: ");
        char word[64];
        scanf("%s", word);
        if(strcmp(word, "-1") == 0) {
            cmdQuit = true;
            continue;
        }
        toLower(word, strlen(word));
        if(hashTableFindKey(hashedDict, word, strlen(word)) != -1) {
            printf("\n\t\t\t\tWord \"%s\" is correct.\n\n\n", word);
        } else {
            char* mostSimilarWord;
            short dist = findMostSimilarWord(hashedDict, word, &mostSimilarWord);
            if(dist <= MAX_DIST_ALLOWED) {

                printf("\n\t\t\t\tWord \"%s\" is incorrect.\n"
                       "\t\t\t\tThe most similar word found is: \"%s\"\n"
                       "\t\t\t\tEdit Distance: %d\n\n",
                        word, mostSimilarWord, dist);
            } else {
                printf("Word \"%s\" does not exist.\n\n", word);
            }
        }
    }
    hashTableFree(hashedDict);
    return 0;
}
/*
   FUNCTION: hashTableLoadFromText
   @param1 fp: File pointer
   @returns a pointer to the created HashTable

   INFO: Creates a HashTable in memory using the information in
         the ASCII formatted dictionary file. Each word in the
         file must be the first word in each line.
*/
HashTable_t* hashTableLoadFromText(FILE* fp) {

    HashTable_t* hashTable = (HashTable_t*) malloc( sizeof(HashTable_t) );
    if(hashTable == NULL) {
        printf("ERROR: Could not allocate memory.\n");
        exit(EXIT_FAILURE);
    }

    hashTable->size = hashTableCalculateOptimalSize( getLineCount(fp) );

    hashTable->table = calloc(hashTable->size, sizeof(char*));

    if(hashTable->table == NULL) {
        printf("ERROR: Could not allocate memory.\n");
        exit(EXIT_FAILURE);
    }

    char buffer[64];
    int index;

    while(!feof(fp)) {
        // Read the first string seperated by a space, ignore the rest of the line
        fscanf(fp, "%s%*[^\n]", buffer);

        // Make all letters lowercase
        toLower(buffer, strlen(buffer));

        // Skip the line if the string is not a word (Contains characters other than a-z)
        if(hasInvalidChars(buffer)) continue;

        // Calculate the hash value
        index = hashTableGetHash(hashTable, buffer, strlen(buffer));

        // If the generated index is full, find the next available space (Open-Addressing)
        while(hashTable->table[index] != NULL) {
            index = (++index) % hashTable->size;
        }

        // An empty cell is found, fill it with the new element.
        hashTable->table[index] = calloc( (strlen(buffer) + 1), sizeof(char) );

        if(hashTable->table[index] == NULL) {
            printf("ERROR: Could not allocate memory.\n");
            exit(EXIT_FAILURE);
        }

        strcpy(hashTable->table[index], buffer);
    }

    return hashTable;
}
/*
*   FUNCTION: hashTableLoadFromBinary
*   @param1 fp: Pointer to the binary file
*   @returns a pointer to the read HashTable
*
*   INFO: Creates a HashTable in memory using the information in the
*         given binary file.
*/
HashTable_t* hashTableLoadFromBinary(FILE* fp) {
    int size, wordCount;

    fread(&size, sizeof(int), 1, fp); // Size of the HashTable.
    fread(&wordCount, sizeof(int), 1, fp); // Number of elements in the HashTable

    HashTable_t* hashTable = malloc( sizeof(HashTable_t) );

    if(hashTable == NULL) {
        printf("ERROR: Could not allocate memory.\n");
        exit(EXIT_FAILURE);
    }

    hashTable->size = size;
    hashTable->table = calloc(size, sizeof(char*));

    if(hashTable->table == NULL) {
        printf("ERROR: Could not allocate memory.\n");
        exit(EXIT_FAILURE);
    }

    int i, addr, len;

    for(i=0; i<wordCount; i++) {

        fread(&addr, sizeof(int), 1, fp);
        fread(&len, sizeof(int), 1, fp);

        hashTable->table[addr] = calloc(len, sizeof(char));

        if(hashTable->table[addr] == NULL) {
            printf("ERROR: Could not allocate memory.\n");
            exit(EXIT_FAILURE);
        }

        fread(hashTable->table[addr], sizeof(char), len, fp);
    }
    return hashTable;

}
/*
*   FUNCTION: hashTableSaveAsBinary
*   @param1 hashedDict: Pointer to the HashTable
*   @param2 filePath: Path of the file to write
*   @returns nothing
*
*   INFO: Saves the given HashTable into the disk as a binary file.
*/
void hashTableSaveAsBinary(HashTable_t* hashTable, const char* filePath) {
    FILE* fp = fopen(filePath, "wb");

    int size = hashTable->size;
    int wordCount = 0;
    int i=0, len;

    for(i=0; i<size; i++) {
        if(hashTable->table[i] != NULL) {
            wordCount++;
        }
    }

    fwrite(&size, sizeof(int), 1, fp); // Size of the HashTable
    fwrite(&wordCount, sizeof(int), 1, fp); // Number of elements in the HashTable

    for(i=0; i<size; i++) {
        if(hashTable->table[i] != NULL) {
            fwrite(&i, sizeof(int), 1, fp); // Address of the word

            len = strlen(hashTable->table[i]) + 1; // +1 for the string terminator
            fwrite(&len, sizeof(int), 1, fp); // Length of the word

            fwrite(hashTable->table[i], sizeof(char), len, fp); // word
        }
    }
    fclose(fp);
}
/*
*   FUNCTION: hashTableFindKey
*   @param1 hashTable: Pointer to the HashTable
*   @param2 key: Key word to look for
*   @param3 keyLen: Length of the word
*   @returns the address if the key exists, -1 otherwise
*
*   INFO: Looks for a given key in the given HashTable.
*/
int hashTableFindKey(HashTable_t* hashTable, char* key, int keyLen) {
    int addr = hashTableGetHash(hashTable, key, keyLen);
    int count = 0;

    while(hashTable->table[addr] != NULL && count < hashTable->size) {
        if(strcmp(hashTable->table[addr], key) == 0) {
            return addr;
        }
        addr = (++addr) % hashTable->size;
        count++;
    }
    return -1;
}
/*
*   FUNCTION: hashTableGetHash
*   @param1 hashTable: Pointer to the HashTable
*   @param2 key: Key to be calculated
*   @param3 keyLen: Length of the key
*   @returns the hash value
*
*   INFO: Calculates the given keys hash value, using Horner's Method.
*         THIS IS DESIGNED TO HASH ONLY LOWERCASE LETTERS
*/
int hashTableGetHash(HashTable_t* hashTable, char* key, int keyLen) {
    const int constant = 31;
    int addr = 0, i;

    for(i=0; i<keyLen; i++) {
        addr = (addr * constant + (key[i] - 96)) % hashTable->size;
    }
    return addr;
}
void hashTableFree(HashTable_t* hashTable) {
    int i;
    for(i=0; i<hashTable->size; i++) {
        free(hashTable->table[i]);
    }
    free(hashTable->table);
    free(hashTable);
}
int hashTableCalculateOptimalSize(int numberOfElements) {
    // Find the nearest prime that is greater than 2x(Number of Elements)
    int M = numberOfElements * 2;
    while(!isPrime(M)) M++;
    return M;
}
/*
*   FUNCTION: findMostSimilarWord
*   @param1 hashTable: Pointer to the dictionary HashTable
*   @param2 key: The word that is being searched
*   @param3(return parameter) wordFound: The pointer that the address
*                                       of the most similar word found
*                                       will be copied into
*   @returns the distance
*
*   INFO: Finds the most similar word to the given key and returns the
*         distance between them. Also returns the word found through
*         @param3.
*/
short findMostSimilarWord(HashTable_t* hashTable, char* key, char** wordFound) {
    char* word;
    short min, dist;
    int i = 0;

    // Advance in the dictionary until a non-null element found.
    while( (word = hashTable->table[i]) == NULL && i < hashTable->size) {
        i++;
    }

    // Verify that the dictionary is not empty.
    if(i == hashTable->size) {
        *wordFound = NULL;
        return -1;
    }

    // Select the first element as minimum.
    min = getEditDistance(word, key);
    *wordFound = word;

    // Find the minimum
    i++;
    for( ; i<hashTable->size; i++) {
        word = hashTable->table[i];
        if(word == NULL) continue;

        dist = getEditDistance(word, key);

        if(dist < min) {
            min = dist;
            *wordFound = word;
        }
    }
    return min;
}
/*
*   FUNCTION: getEditDistanceMatrix
*   @param1 str1: First string
*   @param2 len1: Length of first string
*   @param3 str2: Second string
*   @param4 len2: Length of second string
*   @returns the edit distance matrix
*
*   INFO: Calculates and returns the edit distance matrix.
*/
short** getEditDistanceMatrix(char* str1, short len1, char* str2, short len2) {

    // Create edit distance matrix
    short** ED = malloc(sizeof(short*) * (len1+1));

    if(ED == NULL) {
        printf("ERROR: Could not allocate memory.\n");
        exit(EXIT_FAILURE);
    }

    short i,j;

    for(i = 0; i <= len1; i++) {
        ED[i] = malloc(sizeof(short) * (len2+1));

        if(ED[i] == NULL) {
            printf("ERROR: Could not allocate memory.\n");
            exit(EXIT_FAILURE);
        }

        // Initializing first column
        ED[i][0] = i;
    }
    // Initializing first row
    for(j = 0; j<=len2; j++) {
        ED[0][j] = j;
    }

    for(i = 1; i<=len1; i++) {
        for(j = 1; j<=len2; j++) {

            if(str1[i-1] == str2[j-1]) {
                ED[i][j] = ED[i-1][j-1];
            }
            else {

                ED[i][j] = getMin(ED[i-1][j] + 1,
                                  ED[i][j-1] + 1,
                                  ED[i-1][j-1] + SUBSTITUTION_COST);
            }
        }
    }

    return ED;
}
/*
*   FUNCTION: getEditDistance
*   @param1 str1: First string
*   @param2 str2: Second string
*   @returns the distance
*
*   INFO: Calculates and returns the edit distance between
*         two given strings.
*/
short getEditDistance(char* str1, char* str2) {

    short len1 = strlen(str1);
    short len2 = strlen(str2);
    short** ED = getEditDistanceMatrix(str1, len1, str2, len2);

    // Return the bottom-right element
    int val = ED[len1][len2];
int i;
    for( i = 0;i <= len1; i++) {
        free(ED[i]);
    }
    free(ED);

    return val;
}

// Returns the smallest number among the 3 given numbers
int getMin(int x, int y, int z) {
    if(x < y) {
        if(x < z) return x;
        else return z;
    } else {
        if(y < z) return y;
        else return z;
    }
}

/*
*   FUNCTION: hasInvalidChars
*   @param1 string: String to check for
*   @returns true if the string has characters other than a-z,
*            false otherwise
*
*   INFO: This function is used to verify if a string is a word.
*         A string is considered to be word, if all the characters
*         in the string are a lowercase English letter.
*/
bool hasInvalidChars(char* string) {
    short i;
    for(i=0; i<strlen(string); i++) {
        if(string[i] < 'a' || string[i] > 'z') return true;
    }
    return false;
}
void toLower(char string[], int len) {
    short i;

    for(i=0; i<len; i++) {
        if(string[i] >= 'A' && string[i] <= 'Z') string[i] += 32;
    }
}
bool isPrime(int x) {
    int i;
    for(i = 2; i <= x/2; i++) {
        if(x % i == 0) return false;
    }
    return true;
}
int getLineCount(FILE* fp) {
    int count = 0;
    // Get current position of the file pointer
    long pos = ftell(fp);
    // Set the pointer position to the beginning of the file
    fseek(fp, 0, SEEK_SET);
    // Count newline characters
    while(!feof(fp)) {
        if(fgetc(fp) == '\n') count++;
    }
    // Set the position back where it was before
    fseek(fp, pos, SEEK_SET);
    return count;
}
