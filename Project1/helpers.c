/* Determines if the given string is a number */
bool isnumber(char* num){
	int i;
	for(i=0; i<strlen(num); i++){
		if(isdigit(*(num+i)) == 0)
			return false;
	}
	return true;
}
