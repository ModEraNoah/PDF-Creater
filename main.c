#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "./fonts.c"

#define FONTSIZE 12
#define PAGEWIDTH 595
#define PAGELENGTH 842
#define ROWDISTANCE 1.5

static uint objectCounter = 0;

typedef struct {
	uint objectNumber;
	char *content;
} Object;

char* getStart() {
	return "%PDF-1.7\n";
}


int getDigitStringWidth(int digit) {
	int charCounter = 1;
	while ((int)(digit/(pow(10, charCounter)))) {
		charCounter++;
	}
	return charCounter;
}

void craftObjectContent(int objectNumber, char contentString[],char** res) {

	int charCounter = getDigitStringWidth(objectNumber);

	char objectBeginning[charCounter + 8];
	snprintf(objectBeginning, sizeof(objectBeginning), "%i 0 obj\n", objectNumber);

	char *objectEnding = "\nendobj";

	int length = strlen(objectBeginning) + strlen(contentString) + strlen(objectEnding);
	char *content= (char*) malloc((length+1)  * sizeof(char));
	if (content == NULL) {
		return;
	}
	res[0] = content;

	strcpy(content, objectBeginning);
	strcat(content, contentString);
	strcat(content, objectEnding);
}

Object getPdfRoot(int objectNumber, int pagesObjectNumber) {
	char *tmp = "<</Type /Catalog /Pages %d 0 R>>";
	int len = strlen(tmp) + getDigitStringWidth(pagesObjectNumber) + 1;
	char buf[len];
	snprintf(buf, len, "<</Type /Catalog /Pages %d 0 R>>", pagesObjectNumber);

	char **content = malloc(sizeof(char*));
	craftObjectContent(objectNumber, buf, content);

	Object temp = {objectNumber, *content};
	free(content);
	return temp;
}

Object getPagesRoot(int objectNumber, int count, int kids[]) {
	int sumOfKidsDigits = 0;
	for (int i = 0; i < count; i++) {
		sumOfKidsDigits += getDigitStringWidth(kids[i]);
	}

	int kidsArrayLength = strlen(" 0 R ")*count + sumOfKidsDigits;
	char kidsArray[kidsArrayLength];
	memset(kidsArray,0,kidsArrayLength);
	for (int i = 0; i < count; i++) {
		char tmp[20];
		snprintf(tmp, 20, "%d 0 R ", kids[i]);
		strcat(kidsArray, tmp);
	}
	char str[52 + kidsArrayLength + getDigitStringWidth(count) + getDigitStringWidth(PAGEWIDTH) + getDigitStringWidth(PAGELENGTH)];
	sprintf(str, "<</Type /Pages /Kids [%s] /Count %d /MediaBox [0 0 %d %d]>>",kidsArray, count,PAGEWIDTH, PAGELENGTH);

	char **content = malloc(sizeof(char*));
	craftObjectContent(objectNumber, str, content);

	Object temp = {objectNumber,*content};
	free(content);
	return temp;
}

Object createPageObject(int objectNumber, int parent, int contents) {

	int additionalLen = 0;
	additionalLen = getDigitStringWidth(parent);
	additionalLen += getDigitStringWidth(contents);

	char *str = "<</Type /Page /Parent %d 0 R /Contents %d 0 R>>";
	int contentLength = strlen(str) + 1;
	char contentString[contentLength + additionalLen];
	snprintf(contentString, contentLength, str, parent, contents);

	char **content = malloc(sizeof(char*));
	craftObjectContent(objectNumber, contentString, content);

	Object temp = {objectNumber,*content};
	free(content);
	return temp;
}

int createFormattedText(char** buf, char inputText[]) {
	const int LENGTH = 8192;
	int pageIndex = 0;

	buf[pageIndex] = malloc(LENGTH);
	if (buf[pageIndex] == NULL) {
		return pageIndex + 1;
	}

	strcpy(buf[pageIndex], "");
	int rowCount = 1;
	int characterCount = 0;
	
	char currentWord[120];
	int currentWordCounter = 0;
	strcpy(currentWord, "");
	buf[pageIndex][0] = '(';

	float widthCounter = 0;
	float wordWidth = 0;
	for (int i = 0; inputText[i] != '\0'; i++) {
		if (inputText[i] != ' ') 
			currentWord[currentWordCounter] = inputText[i];

		characterCount++;
		currentWordCounter++;

		float currentWidth = ((float)(TIMES_WIDTH[inputText[i]-32]/1000.0)*FONTSIZE);
		if (currentWidth < 0.0) {
			currentWidth = 8.0; 
		}
		widthCounter += currentWidth;
		wordWidth += currentWidth;
		if (inputText[i] == ' ') {
			char isLongerThanLine = PAGEWIDTH - 100 - FONTSIZE - widthCounter < 0;
			if (isLongerThanLine) {
				strcat(buf[pageIndex], ") ");

				if (rowCount == 1) {
					strcat(buf[pageIndex], "Tj");
				} else {
					strcat(buf[pageIndex], "\'");
				}

				characterCount = 0;
				rowCount++;
				// if (rowCount > 10) {
				if (PAGELENGTH - (rowCount * FONTSIZE * ROWDISTANCE) - 35 <= 0) {
					pageIndex++;
					rowCount = 1;
					buf[pageIndex] = malloc(LENGTH);
					strcpy(buf[pageIndex], "");
					if (buf[pageIndex] == NULL) {
						return pageIndex + 1;
					}
				}

				strcat(buf[pageIndex], "\n(");
				strcat(buf[pageIndex], currentWord);
				widthCounter = wordWidth;

			} else {
				strcat(buf[pageIndex], currentWord);
			}
			strcat(buf[pageIndex], " ");
			memset(currentWord, 0, 100);
			currentWordCounter = 0;
			wordWidth = 0;
		}
	}

	char isTooLongAtEnd = (PAGEWIDTH - (FONTSIZE * 3)) - widthCounter < 0;
	if (!isTooLongAtEnd) {
		strcat(buf[pageIndex], currentWord);
	} 
	strcat(buf[pageIndex], ") ");
	if (rowCount == 1) {
		strcat(buf[pageIndex], "Tj");
	} else {
		strcat(buf[pageIndex], "\'");
	}

	if(isTooLongAtEnd) {
		strcat(buf[pageIndex], "\n(");
		strcat(buf[pageIndex], currentWord);
		strcat(buf[pageIndex], ") \'");
	}
	
	/* +1 as the index starts at 0 but ocunt at 1*/
	return pageIndex+1;
}

Object createTextObject(int objectNumber, char text[]) {
		
	int textLen = strlen(text) + 1;
	char *str = "stream\nBT\n/F1 %d Tf\n%f TL\n30 %d Td\n%s\nET\nendstream";
	int strLength = strlen(str) + 1;
	int streamLength = textLen + strLength + 1 + getDigitStringWidth(FONTSIZE) + getDigitStringWidth(FONTSIZE) * ROWDISTANCE+ getDigitStringWidth(PAGELENGTH);// + 12;
	char stream[streamLength];
	snprintf(stream, streamLength, str, FONTSIZE, FONTSIZE * ROWDISTANCE , PAGELENGTH - FONTSIZE - 20, text);
	
	int charCounter = getDigitStringWidth(streamLength);
	char *dictStr = "<</Length %d>>\n";
	int dictLength = strlen(dictStr) + 1;
	char dict[charCounter + dictLength] ;
	snprintf(dict, dictLength + charCounter, dictStr, streamLength);

	char contentString[streamLength + charCounter + dictLength + 1];
	strcpy(contentString, dict);
	strcat(contentString,stream);

	char **content = malloc(sizeof(char*));
	craftObjectContent(objectNumber, contentString, content);

	Object temp = {objectNumber,*content};
	free(content);
	return temp;
}

static int currentLength = 10;
char** getXref(Object obs[]) {
	char ** t = malloc (1 * sizeof(char*));

	char num[4];
	snprintf(num, sizeof(num), "%d\n", objectCounter+1);
	char temp[6] = "0 ";
	strcat(temp, num);
	t[0] = malloc(1000 * sizeof(char));
	strcpy(t[0], "xref\n");
	strcat(t[0], temp);
	strcat(t[0],"0000000000 65535 f\n");
	for (int i = 0; i < objectCounter; i++) {
		char numb[11];
		snprintf(numb, sizeof(numb), "%010d", (currentLength));
		currentLength += (strlen(obs[i].content) + 1);
		strcat(t[0], numb);
		strcat(t[0], " 00000 n\n");
	}
	strcat(t[0], "trailer\n");
	return t;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("No file specified\n");
		printf("File has to be given with the flag --file\n");
		return -1;
	}

	if (strcmp(argv[1], "--file") != 0) {
		printf("File has to be given with the flag --file\n");
		printf("unknown flag %s", argv[1]);
		return -1;
	}

	char *textInput = malloc(10000 * sizeof(char));
	if (textInput == NULL) {
		printf("some issue with allocating the memory for textInput\n");
		return -1;
	}
	strcpy(textInput, "");

	FILE *file;
	file = fopen(argv[2], "r");

	if (!file) {
		printf("Some issue with the file '%s'!\n", argv[2]);
		return -1;
	} 
	
	int cur;
	int i = 0;
	while ((cur = fgetc(file)) != EOF) {
		textInput[i] = cur;
		i++;
	}
	textInput[i] = '\0';

	fclose(file);

	// int c;
	// int i = 0;
	// while ((c = getchar()) != EOF) {
	// 	textInput[i] = c;
	// 	i++;
	// }

	int maxObjects = 200;
	int obsCounter = 0;
	Object obs[maxObjects];

	int pagesRootNumber = ++objectCounter;
	obs[obsCounter] = getPdfRoot(++objectCounter, pagesRootNumber);
	Object *root = &obs[obsCounter];
	obsCounter++;

	/* maximum of 100 pages */
	char **formattedText = malloc(sizeof(char*)*100);
	if (formattedText == NULL) {
		printf("could not allocate memory for formatted text\n");
	}
	int numberOfPages = createFormattedText(formattedText, textInput);
	free(textInput);

	int kidsArray[numberOfPages]; 

	Object *textObject ;
	for (int i = 0; i < numberOfPages; i++) {
		obs[obsCounter]= createTextObject(++objectCounter, formattedText[i]);
		free(formattedText[i]);
		textObject = &obs[obsCounter];
		obsCounter++;

		obs[obsCounter] = createPageObject(++objectCounter, pagesRootNumber, textObject->objectNumber);
		kidsArray[i] = objectCounter;
		obsCounter++;
	}
	free(formattedText);

	obs[obsCounter] = getPagesRoot(1, sizeof(kidsArray)/sizeof(int), kidsArray);
	obsCounter++;

	printf("%s",getStart());

	for (int i = 0; i < obsCounter; i++) {
		printf("%s\n", obs[i].content);
	}

	char **temp = getXref(obs);
	printf("%s", temp[0]);
	int bufLength = strlen("<</Size %d /Root %d 0 R>>\n") + getDigitStringWidth(objectCounter + 1)+1;
	char buf[bufLength];
	snprintf(buf, bufLength, "<</Size %d /Root %d 0 R>>\n", objectCounter+1, root->objectNumber);
	printf("%s",buf);
	printf("startxref\n");
	printf("%d\n", currentLength);
	printf("%%%%EOF\n");

	// for (int i= 0; i < obsCounter - 1; i++) {
	// 	free(obs[obsCounter].content);
	// }

	for (int i = 0; i < obsCounter; i++) {
		free(obs[i].content);
	}
	free(temp[0]);
	free(temp);

	return 0;
}
