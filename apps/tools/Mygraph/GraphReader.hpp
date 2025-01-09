#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include "config.hpp"
#include "lib/graph/Graph.hpp"
#include "lib/utils.hpp"


extern std::unordered_map<std::string,DatasetSetting> DATASET_SETTING;

class Reader{};

class UndirectedGraphReader:public Reader{
private:
    std::string _filepath;
    std::string _filename;
    int _skipstart = 0;
    int _beginnode = 0;
    bool _appear_twice = false; // 尽量让图appear_twice可以有效提高速度

public:
    UndirectedGraphReader(const std::string& filepath):_filepath(filepath) {
        _filename = split_name(_filepath);
        DatasetSetting ds = DATASET_SETTING.at(_filename);
        if(!ds.directed == false){
            print("Warning: Directed graph using UndirectedGraphReader.");
        }
        _skipstart = ds.skipstart;
        _beginnode = ds.beginnode;
        _appear_twice = ds.appear_twice;
    }

    EdgeGraph* read_from_text(){
        double sec = get_current_time_sec_method();

        FILE* file = std::fopen(_filepath.c_str(), "r");
        if (!file) {
            std::cerr << "Failed to open file!" << std::endl;
            exit(1);
        }
        // 定位到文件末尾
        std::fseek(file, 0, SEEK_END);
        size_t fileSize = std::ftell(file);
        std::rewind(file);

        // 分配内存以保存整个文件内容
        char* fileContent = new char[fileSize + 2]; // 加2是为了存放字符串结尾的 '\0'和'\n'
        
        // 读取文件内容到内存中
        double loc1 = get_current_time_sec_method();
        std::size_t bytesRead = std::fread(fileContent, 1, fileSize, file);
        double loc1time = get_current_time_sec_method() - loc1;
        std::cout << "loc1 time: " << loc1time << std::endl;
        if (bytesRead != fileSize) {
            std::cerr << "Error reading file!" << std::endl;
            std::fclose(file);
            exit(1);
        }
        if(fileContent[fileSize-1] != '\n'){
            fileContent[fileSize] = '\n';
            fileSize++;
        }
        fileContent[fileSize] = '\0'; // 手动添加字符串结尾标志
        std::fclose(file);

        // 对内存中的内容进行逐行处理，先跳过一些行
        char* ptr = fileContent;
        for(int row=0;row<_skipstart;ptr++){
            if(*ptr == '\n'){
                row++;
            }
        }

        // 先读一遍文件找到n，m
        nodeint n = 0;
        nodeint m = 0;
        nodeint current = 0;
        nodeint row = 0;
        double loc2 = get_current_time_sec_method();
        for(char* p = ptr;*p;p++){
            if(*p == '\t' || *p == ' '){
                if(current>n){
                    n = current;
                }
                current = 0;
            } else if(*p == '\n'){
                row++;
                if(current>n){
                    n = current;
                }
                current = 0;
            } else{
                current = 10*current + *p - '0'; 
            }
        }
        double loc2time = get_current_time_sec_method() - loc2;
        std::cout << "loc2 time: " << loc2time << std::endl;

        n = n + 1 - _beginnode;
        m = _appear_twice ? row / 2 : row;

        // 知道了n后再读一遍文件找到g,deg。这一部分是最占用时间的。
        std::vector<std::vector<nodeint>> g(n);
        std::vector<nodeint> deg(n,0);
        int pos = 0;
        nodeint line[2] = {0};
        double loc3 = get_current_time_sec_method();
        for(char* p = ptr;*p;p++){
            if(*p == '\t' || *p == ' '){
                // while(*p == '\t' || *p == ' ') p++;
                pos++;
                line[pos] = 0;
            } else if(*p == '\n'){
                g[line[0]-_beginnode].emplace_back(line[1]-_beginnode);
                deg[line[0]-_beginnode]++;
                if(!_appear_twice){ // 这个分支比外面的速度慢很多，暂时不知道怎么优化。尽量让图appear_twice可以有效提高速度
                    g[line[1]-_beginnode].emplace_back(line[0]-_beginnode);
                    deg[line[1]-_beginnode]++;
                }
                pos = 0;
                line[pos] = 0;
            } else{
                line[pos] = 10*line[pos] + *p - '0'; 
            }
        }
        double loc3time = get_current_time_sec_method() - loc3;
        std::cout << "loc3 time: " << loc3time << std::endl;

        EdgeGraph* G = new EdgeGraph;
        G->g = std::move(g);
        G->deg = std::move(deg);
        G->n = n;
        G->m = m;
        G->name = _filename;
        G->show();

        delete[] fileContent;
        double read_time = get_current_time_sec_method() - sec;
        std::cout << "graph read time: " << read_time << std::endl;
        
        return G;
    }  
};

