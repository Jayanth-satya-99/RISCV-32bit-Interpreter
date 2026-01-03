#pragma once
#include<iostream>
#include<unordered_map>
#include<vector>
#include<string>
#include<fstream>
using namespace std;


//--------------------------------------------------------
string binary_convert_string(int n , int width){
    string out(width , '0');
    unsigned int val = static_cast<unsigned int>(n);
    val = val&((1u<<width)-1);
    for(int i =  width -1 ; i>=0 ; --i){
        out[i]=(val & 1 ) ? '1' : '0';
        val>>=1;
    }
    return out ;}
//--------------------------------------R-type
unordered_map<string,pair<string,string>>umap_R={
  {"add",{"000","0000000"}},
  {"sub",{"000","0100000"}},
  {"xor",{"100","0000000"}},
  {"or",{"110","0000000"}},
  {"and",{"111","0000000"}},
  {"sll",{"001","0000000"}},
  {"srl",{"101","0000000"}},
  {"sra",{"101","0100000"}},
  {"slt",{"010","0000000"}},
  {"sltu",{"011","0000000"}},
  {"mul",{"000","0000001"}},
  {"rem",{"110","0000001"}}
  
};

string R_type(string ins  , int rd , int rs1 , int rs2 ){
    string machine = "";
    machine += umap_R[ins].second;
    machine+=binary_convert_string(rs2,5);
    machine+=binary_convert_string(rs1,5);
    machine+=umap_R[ins].first;
    machine+=binary_convert_string(rd,5);
    machine+="0110011";
    
    return machine ;
}

//--------------------------------I_type
struct I_type_umap_element{
    string func3;
    bool type ;
    string func7;
};

unordered_map<string,I_type_umap_element>umap_I{
    {"addi",{"000",false,""}},
    {"xori",{"100",false,""}},
    {"ori",{"110",false,""}},
    {"andi",{"111",false,""}},
    {"slti",{"010",false,""}},
    {"sltiu",{"011", false ,""}},
    {"slli",{"001",true, "0000000"}},
    {"srli",{"101",true,"0000000"}},
    {"srai",{"101",true,"0100000"}}

};

string I_type(string ins , int rd , int rs1 , int I){
    string machine = "";
    if(umap_I[ins].type){
        machine+=umap_I[ins].func7;
        machine+=binary_convert_string(I ,5);
    }else{
        machine+=binary_convert_string(I ,12);
    }
    machine+=binary_convert_string(rs1,5);
    machine+=umap_I[ins].func3;
    machine+=binary_convert_string(rd,5);
    machine+="0010011";
    return machine ;
}
//------------------------------------------L-type
unordered_map<string,string>umap_L{
    {"lb","000"},
    {"lh","001"},
    {"lw","010"},
    {"lbu","100"},
    {"lhu" , "101"}
};

string L_type(string ins , int rd , int rs1 , int offset){
    string machine = "";
    machine+=binary_convert_string(offset , 12);
    machine+=binary_convert_string(rs1,5);
    machine+=umap_L[ins];
    machine+=binary_convert_string(rd , 5);
    machine+="0000011";
    
    return machine ;
}
//-----------------------------------------S_type
unordered_map<string,string>umap_S{
    {"sb","000"},
    {"sh" , "001"},
    {"sw" ,"010"}
};
string S_type(string ins , int rs2 , int rs1 , int offset){
    string offset_binary=binary_convert_string(offset, 12);
    string machine = offset_binary.substr(0,7);
    machine+=binary_convert_string(rs2,5);
    machine+=binary_convert_string(rs1 , 5);
    machine += umap_S[ins];
    machine += offset_binary.substr(7,5);
    machine+="0100011";
    return machine ;
}
//------------------------------------------------------B_type
unordered_map<string,string>umap_B{
    {"beq","000"},
    {"bne" , "001"},
    {"blt","100"},
    {"bge","101"},
    {"bltu","110"},
    {"bgeu" , "111"}
};

string B_type(string ins , int rs1 , int rs2, int imm){
    
    string imm_bin = binary_convert_string(imm , 12);
    
    string machine = "";
    machine += imm_bin[0];
    machine+=imm_bin.substr(2,6);
    machine+=binary_convert_string(rs2 , 5);
    machine+=binary_convert_string(rs1 , 5);
    machine+=umap_B[ins];
    machine+=imm_bin.substr(8,4);
    machine+=imm_bin[1];
    machine += "1100011";
    
    return machine ;
}

//--------------------------------------------------J-type
string J_type(string ins , int rd , int rs1 , int I){
    string machine = "";
    if(ins == "jal"){
        
        string I_binary = binary_convert_string(I , 20);
    
        machine+= I_binary.substr(0,1);
        machine+= I_binary.substr(10,10);
        machine+=I_binary.substr(9,1);
        machine+= I_binary.substr(1,8);
        machine+=binary_convert_string(rd, 5);
        machine+="1101111";
    }else{ //jalr
        machine += binary_convert_string(I,12);
        machine += binary_convert_string(rs1 , 5);
        machine+="000";
        machine+=binary_convert_string(rd , 5);
        machine +="1100111";
    }
    
    return machine;
}


//----------------------------------
vector<string>parser_helper(string &instruction){
    vector<string> parts ;
    for(auto &c : instruction ){
        if(c==','|| c =='\t')  c= ' ';
    }
    string part;
    for(char c : instruction){
        if(c==' '){
            if(!part.empty()){
                parts.push_back(part);
                part.clear();
            }
        }else part+= c ;
    }
    if(!part.empty()){
        parts.push_back(part);
    }
    return parts ;
}

//--------------------
int get_reg_num(const string &reg){
    static unordered_map<string , int>reg_map={
     {"zero" , 0},{"ra",1},{"sp", 2},{"gp", 3},{"tp", 4},
     {"t0", 5},{"t1",6},{"t2", 7},{"s0",8},{"fp",8},{"s1", 9},
     {"a0",10},{"a1", 11},{"a2",12},{"a3",13},{"a4",14},{"a5",15},
     {"a6",16},{"a7",17},{"s2", 18},{"s3", 19},{"s4", 20},{"s5",21},
     {"s6",22},{"s7",23},{"s8",24},{"s9" , 25},{"s10",26},{"s11",27},
     {"t3", 28},{"t4",29},{"t5",30},{"t6",31}
    };
    
    if(!reg.empty() && reg[0]=='x'){
        return stoi(reg.substr(1));
    }
    if(reg_map.count(reg)){
        return reg_map[reg];
    }
    cerr<<"unknown register : "<<reg<<endl;
    return -1;
}

//-------------------------------------------------parser
string parser(string &instruction){
    vector<string>instruction_set = parser_helper(instruction);
    
    if(instruction_set.empty()) return "";
    
    string output_binary;
    
    if(umap_R.find(instruction_set[0]) !=umap_R.end()){
    if(instruction_set.size()==4){
        int rd = get_reg_num(instruction_set[1]);
        int rs1 = get_reg_num(instruction_set[2]);
        int rs2 = get_reg_num(instruction_set[3]);
        output_binary= R_type(instruction_set[0], rd , rs1 , rs2);
    }else{
        cout<<"R_type instruction is incorrect : "<<instruction<<endl;
    }
    }else if (umap_I.find(instruction_set[0])  !=umap_I.end()){
        if(instruction_set.size()==4){
            int rd = get_reg_num(instruction_set[1]);
            int rs1 = get_reg_num(instruction_set[2]);
            int I = stoi(instruction_set[3]);
            output_binary = I_type(instruction_set[0], rd , rs1 ,I);
        }else{
            cout<<"I_type  instruction is incorrect : "<<instruction<<endl;
        }
    }else if (umap_L.find(instruction_set[0]) != umap_L.end())
    {
        if(instruction_set.size()==3)
        {
            int rd = get_reg_num(instruction_set[1]);
            int b =-1 , c = -1;
            for(int i = 0 ; i< instruction_set[2].size(); i++)
            {
                if(instruction_set[2][i]=='(' && b==-1)  b=i;
                if(instruction_set[2][i]== ')' && c==-1) c=i ;
            }
            if(b>=0 && c>= 0)
            {
                int rs1 = get_reg_num(instruction_set[2].substr(b+1,c-b-1));
                int offset = stoi(instruction_set[2].substr(0,b));
                output_binary=L_type(instruction_set[0], rd , rs1 , offset);
            }
            else{
                cout<<"bracket not found in the instrution : "<<instruction<<endl;
            }
            
        }
        else{
            cout<<"L_type instruction is corect : "<<instruction<<endl;
        }
        
    }
    else if(umap_S.find(instruction_set[0]) !=umap_S.end()){
        if(instruction_set.size()==3){
            int rs2 = get_reg_num(instruction_set[1]);
            int b=-1 , c=-1;
            for(int i = 0 ; i<instruction_set[2].size();i++){
                if(instruction_set[2][i]=='(' && b== -1)  b=i ;
                if(instruction_set[2][i]==')' && c==-1 )   c=i;
            }
            if(b>=0 && c>= 0)
            {
                int rs1 = get_reg_num(instruction_set[2].substr(b+1 , c-b-1));
                int offset = stoi(instruction_set[2].substr(0,b));
                output_binary = S_type(instruction_set[0], rs2 , rs1 , offset);
            }
            else{
                cout<<"bracket not found in instruction : "<<instruction<<endl;
            }
            
        }
        else{
            cout<<"s_type  instruction in incorrect : "<<instruction<<endl;
            
        }
        
    }
    
    else if(umap_B.find (instruction_set[0]) !=umap_B.end())
    {
        if(instruction_set.size()==4){
            int rs1 = get_reg_num(instruction_set[1]);
            int rs2 = get_reg_num(instruction_set[2]);
            int I = stoi(instruction_set[3]);
            output_binary = B_type(instruction_set[0] , rs1 , rs2 , I);
        }
        else{
            cout<<"b_type instruction in wrong : "<<instruction << endl;
        }
    }
    else if(instruction_set[0]=="jal"){
        if(instruction_set.size()==3){
            int rd = get_reg_num(instruction_set[1]);
            int imm = stoi(instruction_set[2]);
            output_binary= J_type("jal",rd,0,imm);
        }
        else{
            cout<<"JAL instruction is wrong"<<instruction<<endl;
        }
    }
    else if(instruction_set[0]=="jalr"){
        if(instruction_set.size()==3){
        int rd=get_reg_num(instruction_set[1]);
        int b=-1,c=-1;
        for(int i=0;i<instruction_set[2].size();i++){
            if(instruction_set[2][i]=='('&&b==-1)b=i;
            if(instruction_set[2][i]==')'&&c==-1)c=i;
        }
        if(b>=0&&c>=0){
            int rd=get_reg_num(instruction_set[1]);
            int rs1=get_reg_num(instruction_set[2].substr(b+1,c-b-1));
            int offset=stoi(instruction_set[2].substr(0,b));
            output_binary=J_type(instruction_set[0],rd,rs1,offset);
        }else{
            cout<<"brackets not found"<<instruction<<endl;
        }
        
        
        }else{
            cout<<"JALR instruction is incorrect "<<instruction<<endl;
        }
    }    
    else{
        cout<<"given instruction is not defined : "<<instruction<<endl;
    }
    
    return output_binary;
}
/*

int main(){
    string filename = "input.txt";
    ifstream infile(filename);
    
    if(!infile.is_open()){
        cerr<<"error : could not open"<<filename<<endl;
        return 1 ;
    }
    string line;
    
    while(getline(infile , line)){
        if(line.empty()) continue;
        
        string machine_code=parser(line);
        if(!machine_code.empty()){
            cout<<line<<"->"<<machine_code<<endl;
        }
    }
    
    infile.close();
    
    return 0;
}*/