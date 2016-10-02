// io beckup

int do_io_redirect(char* filename, char** cmd,char* type){
	pid_t pid = fork();
	mode_t mode = S_IRWXU | S_IRWXO;
	if(pid==0){
		if(type == "<"){
			dup2(open(filename,O_RDONLY),STDIN_FILENO,mode);
		}
		else if(type == ">"){
			dup2(creat(filename,mode),STDOUT_FILENO);
		}
		else if(type == ">>"){
			dup2(open(filename,O_APPEND|O_WRONLY,mode),STDOUT_FILENO);
		}
		execvp(cmd[0],cmd);
		exit(0);
	}
	if(pid == -1){
		perror("Fork Failed");
	}
	else{
		
	}

}