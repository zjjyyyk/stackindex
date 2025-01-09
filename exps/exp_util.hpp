#pragma once

#include <cstdio>
#include <iostream>
#include <fstream> 
#include <sstream>
#include <string>
#include <vector>
#include <functional>
#include <iomanip>
#include <filesystem>
#include <algorithm>



void ensure_dir(std::string dir){
    if (!std::filesystem::exists(dir)){
        std::filesystem::create_directory(dir);
    }
}


void debug_print(const std::vector<double> &res){
    double max_value = res[0];
    int max_index = 0;
    for(int j = 1; j < res.size(); j++){
        if(res[j] > max_value){
            max_value = res[j];
            max_index = j;
        }
    }
    std::cout << "Max value: " << max_value << " at index: " << max_index << std::endl;
}

void generate_singlepair_res(int n, int pairs, std::function<double(int, int)> solver, std::string filename){
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    for(int i = 0; i < pairs; i++){
        int a = rand() % n;
        int b = rand() % n;

        std::cout << i << ". Solving (" << a << " , " << b << ")" << std::endl;
        file << std::setprecision(16) << a << "\t" << b << "\t" << solver(a, b) << std::endl;
    }
    file.close();
    
}

void generate_singlesource_res(int n,int sources,std::function<std::vector<double>(int)> solver, std::string filedir){
    
    ensure_dir(filedir);

    for(int i = 0; i < sources; i++){
        int a = rand() % n;
        std::string filename = filedir + "/" + std::to_string(a);

        std::cout << i << ". Solving (" << a << ")" << std::endl;
        std::vector<double> res = solver(a);

        debug_print(res);

        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        for(int j = 0; j < res.size(); j++){
            file << std::setprecision(16) << res[j] << std::endl;
        }
        file.close();
    }
}

std::vector<std::pair<std::pair<int,int>,double>> read_singlepair_res(std::string filename){
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return std::vector<std::pair<std::pair<int,int>,double>>();
    }

    std::vector<std::pair<std::pair<int,int>,double>> result;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int a, b;
        double value;
        if (!(iss >> a >> b >> value)) {
            std::cerr << "Failed to read line: " << line << std::endl;
            continue;
        }
        result.push_back(std::make_pair(std::make_pair(a, b), value));
    }

    file.close();
    return result;
}

std::vector<std::pair<int,std::vector<double>>> read_singlesource_res(std::string filedir){
    std::vector<std::pair<int,std::vector<double>>> result;

    // Get the list of files in the directory
    std::vector<std::string> files;
    for(const auto & entry : std::filesystem::directory_iterator(filedir)){
        if(entry.is_regular_file()){
            files.push_back(entry.path().string());
        }
    }

    // Read data from each file
    for(const auto & file : files){
        std::ifstream infile(file);
        if (!infile.is_open()) {
            std::cerr << "Failed to open file: " << file << std::endl;
            continue;
        }

        int a = std::stoi(file.substr(file.find_last_of("/\\") + 1));
        std::vector<double> res;
        std::string line;
        while (std::getline(infile, line)) {
            double value = std::stod(line);
            res.push_back(value);
        }

        result.push_back(std::make_pair(a, res));
        infile.close();
    }

    return result;
}

void add_singlepair_res(std::string prev_file, std::function<double(int, int)> solver, std::string output_file){
    std::ifstream infile(prev_file);
    if (!infile.is_open()) {
        std::cerr << "Failed to open file: " << prev_file << std::endl;
        return;
    }

    std::ofstream outfile(output_file);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open file: " << output_file << std::endl;
        infile.close();
        return;
    }

    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        int a, b;
        iss >> a >> b;  

        double new_value = solver(a, b);
        outfile << std::setprecision(16) << line << "\t" << new_value << std::endl;
    }

    infile.close();
    outfile.close();
}

// prev_file: the directory containing the 0 1 2 3 ... files.
void add_singlesource_res(std::string prev_filedir, std::function<std::vector<double>(int)> solver, std::string output_filedir, bool merge = false){

    ensure_dir(output_filedir);

    // Get the list of files in the directory
    std::vector<std::string> files;
    for(const auto & entry : std::filesystem::directory_iterator(prev_filedir)){
        if(entry.is_regular_file()){
            files.push_back(entry.path().string());
        }
    }

    // Read data from each file
    for(const auto & file : files){
        std::ifstream infile(file);
        if (!infile.is_open()) {
            std::cerr << "Failed to open file: " << file << std::endl;
            continue;
        }

        int a = std::stoi(file.substr(file.find_last_of("/\\") + 1));
        std::vector<double> res;
        std::string line;
        while (std::getline(infile, line)) {
            double value = std::stod(line);
            res.push_back(value);
        }

        std::vector<double> new_res = solver(a);

        std::ofstream outfile(output_filedir + "/" + std::to_string(a));
        if (!outfile.is_open()) {
            std::cerr << "Failed to open file: " << output_filedir + "/" + std::to_string(a) << std::endl;
            infile.close();
            continue;
        }

        for(int i = 0; i < new_res.size(); i++){
            if(merge){
                outfile << std::setprecision(16) << res[i] << "\t" << new_res[i] << std::endl;
            } else{
                outfile << std::setprecision(16) << new_res[i] << std::endl;
            }
        }

        infile.close();
        outfile.close();
    }
}

std::vector<double> read_column(std::string filename, int col){
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return {}; // 返回一个空的vector
    }

    std::vector<double> column_values;
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        double value;
        // 跳过前col个值(col从0开始计数)
        for (int i = 0; i < col; ++i) {
            iss >> value;
        }
        // 读取第col个值
        iss >> value;
        if (!iss.fail()) {
            column_values.push_back(value);
        } else{
            std::cerr << "Failed to read line: " << line << std::endl;
        }
    }

    infile.close();
    return column_values;
}

double avg_singlepair_err(std::vector<double> res1, std::vector<double> res2, std::function<double(double,double)> ferr){ // res1和res2 可以通过read_column函数得到
    if(res1.size() != res2.size()){
        std::cerr << "The two results have different sizes" << std::endl;
        return -1;
    }

    double sum = 0;
    for(int i = 0; i < res1.size(); i++){
        sum += ferr(res1[i], res2[i]);
    }

    return sum / res1.size();
}

double avg_singlesource_err(std::vector<std::pair<int,std::vector<double>>> res1, std::vector<std::pair<int,std::vector<double>>> res2, std::function<double(std::vector<double>,std::vector<double>)> ferr){
    if(res1.size() != res2.size()){
        std::cerr << "The two results have different sizes" << std::endl;
        return -1;
    }

    double sum = 0;
    for(int i = 0; i < res1.size(); i++){
        if(res1[i].first != res2[i].first){
            std::cerr << "The two results have different sources" << std::endl;
            return -1;
        }
        sum += ferr(res1[i].second, res2[i].second);
    }

    return sum / res1.size();
}


double l1_err(std::vector<double> res1, std::vector<double> res2){
    double sum = 0;
    for(int i = 0; i < res1.size(); i++){
        sum += std::abs(res1[i] - res2[i]);
    }
    return sum;
}

double rerr(std::vector<double> res1, std::vector<double> res2){
    double sum = 0;
    for(int i = 0; i < res1.size(); i++){
        sum += std::abs(res1[i] - res2[i]) / res1[i];
    }
    return sum/res1.size();
}

template <typename T>
std::vector<T> read_done(std::string filepath){
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        return {}; // 返回一个空的vector
    }

    std::vector<T> column_values;
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        T value;
        if (!(iss >> value)) {
            std::cerr << "Failed to read line: " << line << std::endl;
            continue;
        }
        column_values.push_back(value);
    }

    infile.close();
    return column_values;
}

double avg(std::vector<double> xs){
    double sum = 0;
    for(auto x:xs){
        sum += x;
    }
    return sum / xs.size();
}

bool d_is_in(double x, std::vector<double> xs, double tol = 1e-7){
    for(auto ele: xs){
        if(std::abs(x-ele) < tol){
            return true;
        }
    }
    return false;
}

std::string formatFloat(double value, int precision = 2) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

std::vector<double> generateGeometricSequence(double a, double b, int num) {
    std::vector<double> sequence;
    
    // If num is 1, just return the value 'a'
    if (num == 1) {
        sequence.push_back(a);
        return sequence;
    }

    // Common ratio r calculation
    double r = std::pow(b / a, 1.0 / (num - 1));

    // Generating the geometric sequence
    for (int i = 0; i < num; ++i) {
        sequence.push_back(a * std::pow(r, i));
    }

    return sequence;
}


