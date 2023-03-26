#include <iostream>
#include <unistd.h>
#include "stdio.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include "parser.h"
#include <string>
#include <vector>

struct Process {
    char* name;
    std::vector<char*> args;
};

class Bundle {
    public:
        char* bundle_name;
        char* inputDir;
        char* outputDir;
        char* RepeaterInput;
        std::vector<Process> processes;

};

class Repeater {
    public:
        Bundle* inputBundle;
        Bundle* outputBundle;
        char* content;
    
        Repeater(Bundle* bun1, Bundle* bun2) {
            inputBundle = bun1;
            outputBundle = bun2;
        }
};


Bundle* currBundle;
std::vector<Bundle*> bundles;
std::vector<Repeater*> repeaters;


int len(char *ptr)
{
    int offset = 0;
    int count = 0;
    while (*(ptr + offset) != '\0')
    {
        ++count;
        ++offset;
    }
    
    return count;
}


void executeBundle(Bundle* source) {
    int childCount = 0;
    int execCondition;
    if(source->RepeaterInput) {
        

        if(source->outputDir) {
            int outputfd = open(source->outputDir,O_WRONLY | O_APPEND | O_CREAT,0666);
            dup2(outputfd,1);
            close(outputfd); 
        }

        for (std::vector<Process>::const_iterator i = source->processes.begin(); i != source->processes.end(); ++i) {

            if(fork()) {
                childCount++;
            } else {
                int fd[2];
                pipe(fd);

                if(fork()){
                    close(fd[1]);
                    dup2(fd[0], 0);
                    close(fd[0]);
                    wait(&execCondition);
                    char* argvs[(*i).args.size()+1];
                    for (int j = 0; j < (*i).args.size()+1; j++){
                        if (j != (*i).args.size()){
                            argvs[j] = (*i).args[j];
                        } else {
                            argvs[j] = NULL;
                        }
                        
                    }
                    execvp((*i).name,argvs);
                } else {
                    close(fd[0]);
                    int size = len(source->RepeaterInput);
                    write(fd[1], source->RepeaterInput, size);
                    close(fd[1]);
                    exit(0);
                }
            }
            
        }

    } else {
        if ( source->inputDir != nullptr && source->outputDir != nullptr) {
            for (std::vector<Process>::const_iterator i = source->processes.begin(); i != source->processes.end(); ++i) {
                if(fork()){
                    childCount++;
                } else {
                    int inputfd = open(source->inputDir,O_RDONLY,0666);
                    int outputfd = open(source->outputDir,O_WRONLY | O_APPEND | O_CREAT,0666);
                    dup2(outputfd,1);
                    dup2(inputfd,0);
                    close(inputfd);
                    close(outputfd);
                    char* argvs[(*i).args.size()+1];
                    for (int j = 0; j < (*i).args.size()+1; j++){
                        if (j != (*i).args.size()){
                            argvs[j] = (*i).args[j];
                        } else {
                            argvs[j] = NULL;
                        }
                        
                    }

                    execvp((*i).name,argvs);
                }
            }

        } else if ( source->inputDir != nullptr && source->outputDir == nullptr) {
            for (std::vector<Process>::const_iterator i = source->processes.begin(); i != source->processes.end(); ++i) {
                if(fork()){
                    childCount++;
                } else {
                    int inputfd = open(source->inputDir,O_RDONLY,0666);
                    dup2(inputfd,0);
                    close(inputfd);
                    char* argvs[(*i).args.size()+1];
                    for (int j = 0; j < (*i).args.size()+1; j++){
                        if (j != (*i).args.size()){
                            argvs[j] = (*i).args[j];
                        } else {
                            argvs[j] = NULL;
                        }
                        
                    }

                    execvp((*i).name,argvs);
                }
            }
                
        } else if( source->inputDir == nullptr && source->outputDir != nullptr) {
            for (std::vector<Process>::const_iterator i = source->processes.begin(); i != source->processes.end(); ++i) {
                if(fork()){
                    childCount++;
                } else {
                    int outputfd = open(source->outputDir,O_WRONLY | O_APPEND | O_CREAT,0666);
                    dup2(outputfd,1); 
                    close(outputfd);
                    char* argvs[(*i).args.size()+1];
                    for (int j = 0; j < (*i).args.size()+1; j++){
                        if (j != (*i).args.size()){
                            argvs[j] = (*i).args[j];
                        } else {
                            argvs[j] = NULL;
                        }
                        
                    }

                    execvp((*i).name,argvs);
                }
            }

        } else {
            for (std::vector<Process>::const_iterator i = source->processes.begin(); i != source->processes.end(); ++i) {
                if(fork()){
                    childCount++;
                } else {
                    char* argvs[(*i).args.size()+1];
                    for (int j = 0; j < (*i).args.size()+1; j++){
                        if (j != (*i).args.size()){
                            argvs[j] = (*i).args[j];
                        } else {
                            argvs[j] = NULL;
                        }
                        
                    }

                    execvp((*i).name,argvs);
                }
            }

        }
    }
    

    while( childCount > 0){
        wait(&execCondition);
        childCount--;
    }

}

void repeaterExecute(Repeater* repeater){
    int inputProcSize = repeater->inputBundle->processes.size();
    int fd[2];
    int execCondition;
    pipe(fd);
    
    if(fork()){
        close(fd[1]);
        wait(&execCondition);
        std::string buffer;
        int n = 0;
        char readBuffer[10];
        while ((n = read(fd[0], readBuffer, sizeof(readBuffer))) > 0) {
            for (int i = 0; i < n; i++) {
                buffer.push_back(readBuffer[i]);
            }
        }
        char* charBuffer = new char[buffer.size() + 1];
        std::copy(buffer.begin(), buffer.end(), charBuffer);
        charBuffer[buffer.size()] = '\0';
        repeater->content = charBuffer;
        repeater->outputBundle->RepeaterInput = repeater->content;
        close(fd[0]);
    } else {
        close(fd[0]);
        dup2(fd[1], 1);
        close(fd[1]);
        executeBundle(repeater->inputBundle);
        exit(0);
    }
}


int main() {
    std::string line;
    bool isCreation = 0;
    while(std::getline(std::cin, line) && line.compare("quit")) {
        parsed_input *input = new parsed_input ;
        char* cline = new char[line.size() + 2];
        std::copy(line.begin(), line.end(), cline);
        cline[line.size()] = '\n';
        cline[line.size()+1] = '\0';
        parse(cline,isCreation,input);

        if(input->command.type == PROCESS_BUNDLE_CREATE){
            isCreation = 1;
            currBundle = new Bundle;
            currBundle->bundle_name = input->command.bundle_name;

        } else if (input->command.type == PROCESS_BUNDLE_STOP) {

                isCreation = 0;
                bundles.push_back(currBundle);
                currBundle = nullptr;

        } else if (input->command.type == PROCESS_BUNDLE_EXECUTION){
                bundle_execution* execBundles = input->command.bundles;
                int execbundleCount = input->command.bundle_count;
                int bundleSize = bundles.size();
                for (int i = 0; i < execbundleCount; i++){
                    for(int j = 0; j < bundleSize;j++){
                        if(strcmp(execBundles[i].name,bundles[j]->bundle_name) == 0){
                            bundles[j]->inputDir = execBundles[i].input;
                            bundles[j]->outputDir = execBundles[i].output;
                        }
                    }
                }
                for (int i = 0; i + 1 < execbundleCount; i++){
                    Bundle* inputBundle;
                    Bundle* outputBundle;
                    for(int j = 0; j < bundleSize;j++){
                        if(strcmp(execBundles[i].name,bundles[j]->bundle_name) == 0){
                            inputBundle = bundles[j];
                        }
                        if(strcmp(execBundles[i+1].name,bundles[j]->bundle_name) == 0){
                            outputBundle = bundles[j];
                        }
                    }
                    Repeater* rptr = new Repeater(inputBundle,outputBundle);
                    repeaters.push_back(rptr);
                }
                
                for (int i = 0; i < repeaters.size(); i++) {
                    repeaterExecute(repeaters[i]);
                }
                for(int j = 0; j < bundleSize;j++){
                    if(strcmp(execBundles[execbundleCount-1].name,bundles[j]->bundle_name) == 0){
                        executeBundle(bundles[j]);
                    }
                }
                for (int i =0; i< repeaters.size();i++) {
                    delete (repeaters[i]);
                } 
                repeaters.clear();
                

        } else {

                Process p;
                p.name = input->argv[0];
                char** ptr = input->argv; 
                for (char* c = *ptr; c; c=*++ptr) {
                    p.args.push_back(c);
                }
                currBundle->processes.push_back(p);

        }
        delete[] cline; 
        delete input;        
        
    }

    for (int i =0; i< bundles.size();i++) {
        delete (bundles[i]);
    } 
    bundles.clear();

    if (currBundle){
        delete currBundle;
    }
}



