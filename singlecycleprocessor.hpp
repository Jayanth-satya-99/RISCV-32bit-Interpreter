#pragma once
#include "assembler.hpp"
#include<iostream>
#include<vector>
#include<cstdint>
#include<fstream>
#include<string>



using namespace std;
#define avgsize 32

uint32_t signedExtend(uint32_t imm,int N){
    uint32_t signbit=1<<(N-1);
    signbit=signbit & imm;
    if(signbit!=0){
     
        return imm | (~((1u << N )-1));
    }else{
        return imm;
    }
    
}

uint32_t unsignedExtract(uint32_t ins,int h,int l){
    uint32_t carbon=(1<<(h-l+1))-1;
    ins=(ins>>l)&carbon;
    
    return ins;
}

uint32_t genImm(uint32_t ins)
{
    uint32_t opcode=unsignedExtract(ins,6,0);
    uint32_t imm;
    switch(opcode){
    case 19 : {
    
        if(unsignedExtract(ins , 13 , 11)==1 || unsignedExtract(ins , 13 , 11)== 5)
        {
            imm = unsignedExtract(ins , 24 , 20);
        }
        else
        {
            imm = unsignedExtract(ins , 31 , 20 );
            imm = signedExtend(imm , 12);
        }
        break;
    }
    case 3:
    {
        imm = unsignedExtract(ins , 31 , 20);
        imm = signedExtend(imm , 12);
        break;
    }
    case 35 : 
    {
        uint32_t buff = unsignedExtract(ins , 31 , 25);
        uint32_t buff1 = unsignedExtract(ins , 11 , 7);
        buff= buff<<5;
        
        imm = buff | buff1;
        imm = signedExtend(imm , 12 );
        break;
    }
    case 99 : 
    {
        //there may be a change
        imm = unsignedExtract(ins , 31 , 31);
        imm = (imm <<1)| unsignedExtract(ins,7 ,7);
        imm = (imm << 6)| unsignedExtract(ins, 30 , 25);
        imm = (imm << 4)|unsignedExtract(ins , 11 , 8);
        imm = signedExtend(imm ,12 );
        break;
    }
    case 103:
    {
        imm = unsignedExtract(ins , 31 , 20);
        imm = signedExtend(imm , 12);
        break;
    }
    case 111:
    {
        imm = unsignedExtract(ins , 31 , 31);
        imm = (imm << 8) | unsignedExtract(ins , 19 , 12);
        imm = (imm << 1) | unsignedExtract(ins , 20 , 20);
        imm = (imm << 10 )| unsignedExtract(ins , 30 , 21);
        imm = signedExtend(imm , 20);
        break;
    }
}
return imm ;
}

uint32_t alucontrol(uint32_t func3,uint32_t func7,uint32_t opcode){
    uint32_t word=2;
   switch(opcode){
        case 51 :
                if(func7==0){
                    switch(func3){
                        case 0:word=2;break;
                        case 1:word=4;break;
                        case 2:word=8;break;
                        case 3:word=9;break;
                        case 4:word=3;break;
                        case 5: word=5;break;
                        case 6:word=1;break;
                        case 7:word=0;break;
                    }
                }else if(func7==32){
                    switch(func3){
                        case 0:word=6;break;
                        case 5:word=7;break;
                    }
                }else{
                    word=10;
                }
            
              break;
        case 19:
            if(func3==1||func3==5){
                if(func3==1){
                    word=4;
                }else{
                    if(func7==0){
                        word=5;
                    }
                    if(func7==32){
                        word=7;
                    }
                }
                
            }else{
                switch(func3){
                    case 0:word =2;break;
                    case 2:word=8;break;
                    case 3:word=9;break;
                    case 4:word=3;break;
                    case 6:word=1;break;
                    case 7:word=0;break;
                }
            }
            
            break;
        
        case 99 :
            if(func3==0||func3==1){
                word=6;
            }else if(func3==4||func3==5){
                word=8;
            }else if(func3==6||func3==7){
                word=9;
            }
            
            break;
        
    } 
    return word;
}


struct controlword{
    bool Regwrite = 0 ;
    bool RegRead = 0;
    bool ALUScr = 0;
    uint32_t ALUOP = 0;
    bool Branch = 0;
    uint32_t ALUSelect = 0 ;
    bool MemWrite = 0 ;
    bool MemRead= 0;
    bool Mem2reg = 0;
    bool jump = 0;
    bool jalr = 0;
};




controlword controlunit(uint32_t opcode)
{
    controlword cw ;
    switch(opcode){
        case 51 :
            cw={1,1,0,2,0,0,0,0,0,0,0};
              break;
        case 19:
            cw= {1,1,1,2,0,0,0,0,0,0,0};
            break;
        case 3 :
            cw = {1,1,1,0,0,0,0,1,1,0,0};
            break;
        case 35:
            cw={0,1,1,0,0,0,1,0,0,0,0};
            break;
        case 99 :
            cw = {0,1,0,1,1,0,0,0,0,0,0}    ;
            break;
        case 103:
            cw = {1,1,1,0,0,0,0,0,0,0,1};
            break;
        
        case 111 :
            cw = {1,0,0,0,0,0,0,0,0,1,0};
            break;
    }
    return cw;
}
struct aluout{
    uint32_t result=0;
    bool zero=0;
    bool less=0;
    bool carry=0;
};
aluout ALU(uint32_t src1,uint32_t src2,uint32_t word){
    aluout out;
    switch (word){
        case 0:
            out.result=src1&src2;//and
            break;
        case 1:
            out.result=src1|src2;//or
            break;
        case 2:
            out.result=(int32_t)src1+(int32_t)src2;//add
            break;
        case 3:
            out.result=src1^src2;//xor
            break;
        case 4:
            out.result=src1<<src2;//sll
            break;
        case 5:
            out.result=src1>>src2;//srl
            break;
        case 6:
            out.result=(int32_t)src1-(int32_t)src2;//sub,beq,bne
            if(out.result==0){
                out.zero=1;
            }
            break;
        case 7:
            out.result=(uint32_t)((int32_t)src1>>(int32_t)src2);//sra
            break;
        case 8://slt,blt,bge
            if((int32_t)src1<(int32_t)src2){
                out.result=1;
                out.less=1;
            }
            break;
        case 9://sltu,bltu,bgeu
            if(src1<src2){
                out.result=1;
                out.carry=1;
            }
            break;
        case 10:

            out.result=src1*src2;//mul
            break;
            
            
        
    }
    return out;
}

bool branchunit(aluout out,uint32_t func3, bool branch){
    if(!branch)return false;
    switch(func3){
        case 0 : return out.zero;
        case 1 : return !out.zero;
        case 4 : return out.less;
        case 5 : return !out.less;
        case 6 : return out.carry;
        case 7 : return !out.carry;
        default : return false ;
        
    }
    
}

vector<uint32_t>GPR(32,0);
vector<uint32_t>memory(4*avgsize,0);
vector<uint32_t>IM;
vector<string> IMS;

uint32_t stringtonum(string ins){
    uint32_t out = 0 ;
    for(int i = 0 ; i <ins.size();i++){
        out=out<<1;
        if(ins[i]=='1') out |= 1;
    }
    return out;
}

void loadIM(){
    //string filename = "sumofarray.txt";
    string filename = "factorial.txt";
    //string filename = "input.txt";
    ifstream infile(filename);
    
    if(!infile.is_open()){
        cerr<<"error : could not open"<<filename<<endl;
        return ;
    }
    string line;
    
    while(getline(infile , line)){
        if(line.empty()) continue;
        
        string machine_code=parser(line);
        if(!machine_code.empty()){
            IMS.push_back(machine_code);
            uint32_t insnum = stringtonum(machine_code);
            IM.push_back(insnum);
        }
    }
    
    infile.close();
    
}

void printStatus(){
    cout<<"\n------------ cpu status --------\n";
    cout<<"registers :\n ";
    for(int i = 0 ; i < 32 ; i++){
        cout<<"x"<<i<<" = "<<(int32_t)GPR[i]<<"\n"; //changes done
    }
    cout<<"\n memory : \n";
    for(int i = 0 ; i< memory.size();i++){
        cout<<"MEM["<<i-2*avgsize<<"]"<<(int32_t)memory[i]<<"\n"; //changes done
    }
    cout<<"----------\n";
}






void processor(){
    memory[2*avgsize]=5;
     memory[2*avgsize+1]=1;
      memory[2*avgsize+2]=2;
       memory[2*avgsize+3]=3;
        memory[2*avgsize+4]=4;
    uint32_t PC,INS,NPC,BPC,TPC , JPC,rsl1,rsl2 ,rdl,opcode,func3,func7,imm;
    controlword cw;
    uint32_t rs1,rs2,alusrc1,alusrc2;
    aluout aluresult;
    uint32_t LDresult;
    
    loadIM();
    PC=0;
    
    while((int32_t)PC>=0 && (int32_t)PC< IM.size()){
        INS = IM[(int32_t)PC];
         cout<<PC<<endl;
         
         NPC=PC+1;
         
         JPC=PC+signedExtend(genImm(INS),20);
         rsl1=unsignedExtract(INS , 19 , 15);
         rsl2=unsignedExtract(INS , 24 , 20);
         rdl = unsignedExtract(INS , 11 , 7);
         opcode = unsignedExtract(INS , 6 ,0 );
         func3 = unsignedExtract(INS , 14 , 12);
         func7 = unsignedExtract(INS, 31 , 25);
         imm = genImm(INS);
         cw = controlunit(opcode);
         
         if(cw.RegRead)
         {
             rs1 = GPR[rsl1];
             rs2 = GPR[rsl2];
         }
         alusrc1 = rs1;
         alusrc2 =  (cw.ALUScr) ? imm : rs2;
         cw.ALUSelect= alucontrol(func3 , func7 , opcode);
         aluresult = ALU(alusrc1, alusrc2 , cw.ALUSelect);
         
         BPC = PC + signedExtend(imm , 12 );
         TPC = (branchunit(aluresult , func3 , cw.Branch)) ? BPC : NPC;
         if (cw.jump) TPC = JPC;
         if (cw.jalr) TPC = aluresult.result;
         
         if(cw.MemRead) LDresult = memory[aluresult.result+2*avgsize]; //
         if(cw.MemWrite) memory[aluresult.result+2*avgsize]=rs2;      //
         
         if(cw.Regwrite){
             if(cw.Mem2reg) GPR[rdl]= LDresult;
             else if (cw.jalr || cw.jump) GPR[rdl]=NPC;
             else GPR[rdl] = aluresult.result;
         }
         cout<<IMS[(int32_t)PC]<<endl;
         PC=TPC;
         cout<<PC<<endl;
    }
         printStatus();
    
}

/*int main(){
    processor();
    return 0 ;
}
*/












