#include"singlecycleprocessor.hpp"
#include<iostream>
#include<vector>
#include<cstdint>
#include<fstream>
#include<string>
#include<stdexcept>

using namespace std;
struct PCR{
    uint32_t pc= 1;
    bool pcchanged=false;
    bool valid = false;
};

struct IFID{
    uint32_t pc = 0;
 
    uint32_t IR = 0;
    bool stall = false ;
    bool valid = false;
};

struct IDEX {
    bool stall = false ;
    bool valid = false ;
    bool l1 = false ;
    bool l2 = false ;
    
    uint32_t pc =0, jalpc=0 , imm=0 ;
    controlword CW{};
    uint32_t rs1=0 , rs2=0 , opcode=0 , func3=0 , func7=0 , rsl1=0 , rsl2=0, rdl=0;
};

struct EXMO {
    bool stall = false ;
    bool valid = false;
    uint32_t pc=0; 
    controlword CW{} ;
    uint32_t aluout=0;
    uint32_t rs2=0,rdl=0;
    bool flush=false;
};

struct MOWB {
    bool stall= false;
    bool valid = false ;
    uint32_t pc =0;
    controlword CW{} ;
    uint32_t LDout =0,aluout=0,rdl=0;
   
};

class ins_mem {
     public :
    vector<uint32_t>IM;
    vector<string>IMS;
    int size=0;
   
    ins_mem(){
        IM.push_back(0);
        IMS.push_back("start");
        size+=1;
        
    }
    void write(uint32_t num , string numstr){
        IM.push_back(num);
        IMS.push_back(numstr);
        size+=1;
    }
    uint32_t read(uint32_t  pc){
        if((int32_t)pc>=0 && (int32_t)pc<IM.size()){
            return IM[pc];
        }
        return 0;
    }
};

class GPR{
    struct reg{
        uint32_t value = 0 ;
        uint32_t lock = 0;
    };
    vector<reg>gpr;
    public:
    GPR() : gpr(32){};
    uint32_t read(uint32_t index){
        if(index<32){
            return gpr[index].value;
        }
        return 0;
    }
    
    void write(uint32_t index , uint32_t data){
        if(index>0 && index<32){
            gpr[index].value=data ;
        }
    }
    
    void lock (uint32_t index , uint32_t lockid){
        if(index>0 && index<32){
            gpr[index].lock = lockid;
        }
    }
    
    void unlock(uint32_t index , uint32_t lockid){
        if(index>0 && index<32&&gpr[index].lock == lockid){
            
                gpr[index].lock = 0;
            
        }
    }
    bool islock (uint32_t index){
        if(index<32){
         return gpr[index].lock != 0 ;   
        }
        return false;
        
    }
    uint32_t lockid(uint32_t index){
        if(index<32){
            return gpr[index].lock;
        }
        return 0;
    }
};

class IF{
   
    public :
     class ins_mem IM ;
    void step(PCR &pc , PCR &next_pcr,IFID &next_ifid){
        if(!pc.valid){
            next_ifid.valid=false;
            next_ifid.IR=0;
            return;
        }
        uint32_t ir=IM.read(pc.pc);
        if(ir==0){
            if(!next_pcr.pcchanged){
                next_pcr=pc;
                next_pcr.valid=false;
            }
            next_ifid.valid=false;
            next_ifid.IR=0;
            return;
        }
        next_ifid.IR=ir;
        next_ifid.pc=pc.pc;
        next_ifid.valid=true;
        
        if(!next_pcr.pcchanged){
            next_pcr=pc;
            next_pcr.pc=pc.pc+1;///next line brackets()
            next_pcr.valid=((((int)IM.size)>(int)next_pcr.pc)&&(next_pcr.pc>0));
        }
        
    }
};

class ID {

    uint32_t opcode =0, rsl1=0 , rsl2=0 , rdl=0,INS=0 , jalpc=0;
    controlword cw{} ;
    public :
        class GPR gpr ;
    void step( IFID &ifid , IDEX &next_idex){
        if(!ifid.valid){
            next_idex.valid=false;
            
            return ;
        }
            INS = ifid.IR;
            jalpc= ifid.pc+signedExtend(genImm(INS),20);
            rsl1=unsignedExtract(INS , 19 , 15);
            rsl2=unsignedExtract(INS , 24 , 20);
            rdl = unsignedExtract(INS , 11 , 7);
           
            next_idex.func3=unsignedExtract(INS , 14 , 12);
            next_idex.func7=unsignedExtract(INS , 31 , 25);
            next_idex.imm = genImm(INS);
            cw= controlunit(unsignedExtract(INS,6,0));
             next_idex.opcode = unsignedExtract(INS , 6 , 0);
             next_idex.CW=cw;
             next_idex.CW.ALUOP=next_idex.opcode;/////
            next_idex.l1=gpr.islock(rsl1);
            next_idex.l2=gpr.islock(rsl2);
             if(cw.RegRead){
                 if(next_idex.l1){
                     next_idex.rsl1=rsl1;
                 }else{
                     next_idex.rs1=gpr.read(rsl1);
                 }
                 if(next_idex.l2){
                     next_idex.rsl2=rsl2;
                 }else{
                     next_idex.rs2=gpr.read(rsl2);
                 }
             }
             
             
             if(cw.Regwrite){
                next_idex.rdl =rdl;
                 gpr.lock(rdl, ifid.pc);
             }
             next_idex.jalpc=jalpc;
             next_idex.pc=ifid.pc;
             next_idex.valid=true;
        
    }
};

class EX {
    uint32_t src1=0 , src2=0 ;
    aluout result{};
    uint32_t bpc=0,tpc=0;
    bool branchtaken=false;
    
    public:
    void step(IF &If,PCR &next_pcr,IFID &next_ifid,IDEX &idex,IDEX &next_idex,const EXMO &cur_exmo,EXMO &next_exmo,const MOWB &cur_mowb,bool &stall_requested){
        
        stall_requested=false;
        if(!idex.valid){
            next_exmo.valid=false;
            return;
        }
        bool needs_rs2=false;
        if(idex.opcode==51||idex.opcode==35||idex.opcode==99)needs_rs2=true;
        if(!idex.l1){
            src1=idex.rs1;
        }else{
            bool forwarded=false;
            if(cur_exmo.valid&&cur_exmo.rdl==idex.rsl1&&!cur_exmo.CW.Mem2reg){
                src1=cur_exmo.aluout;
                forwarded=true;
            }
            if(!forwarded&&cur_mowb.valid&&cur_mowb.rdl==idex.rsl1){
                src1=cur_mowb.CW.Mem2reg ? cur_mowb.LDout:cur_mowb.aluout;
                forwarded=true;
            }
            if(!forwarded){
                stall_requested=true;
                next_exmo.valid=false;
                next_idex=idex;
                return;
            }
        }
        
        if(needs_rs2){
            if(!idex.l2){
            src2=idex.rs2;
        }else{
            bool forwarded=false;
            if(cur_exmo.valid&&cur_exmo.rdl==idex.rsl2&&!cur_exmo.CW.Mem2reg){
                src2=cur_exmo.aluout;
                forwarded=true;
            }
            if(!forwarded&&cur_mowb.valid&&cur_mowb.rdl==idex.rsl2){
                src2=cur_mowb.CW.Mem2reg ? cur_mowb.LDout:cur_mowb.aluout;
                forwarded=true;
            }
            if(!forwarded){
                stall_requested=true;
                next_exmo.valid=false;
                next_idex=idex;
                return;
            }
        }
        }else{
            src2=idex.imm;
        }
        next_exmo.rs2=src2;
        if(idex.opcode==35){
            src2=idex.imm;
        }
        next_exmo.CW=idex.CW;
        next_exmo.CW.ALUSelect=alucontrol(idex.func3,idex.func7,idex.opcode);
        result=ALU(src1,src2,next_exmo.CW.ALUSelect);
        branchtaken=branchunit(result,idex.func3,idex.CW.Branch);
        bpc=idex.pc+signedExtend(idex.imm,12);
        tpc=branchtaken?bpc:(idex.pc+1);
        if(idex.CW.jump)tpc=idex.jalpc;
        if(idex.CW.jalr)tpc=result.result;
        next_pcr.pc=tpc;
        next_pcr.valid=((int)If.IM.size>(int)next_pcr.pc)&&next_pcr.pc>0;
        next_pcr.pcchanged=(next_pcr.pc!=(idex.pc+1));
        next_exmo.flush=(branchtaken||idex.CW.jump||idex.CW.jalr);
        if(next_exmo.flush){
            next_ifid.valid=false;
            next_idex.valid=false;
        }
        next_exmo.aluout=result.result;
        if(idex.CW.jump||idex.CW.jalr){
            next_exmo.aluout=idex.jalpc;
        }
        next_exmo.rdl=idex.rdl;
        next_exmo.pc=idex.pc;
        next_exmo.CW=idex.CW;
        next_exmo.valid=true;
        next_idex.valid=false;
        }
};

class memory{
    public:
    vector<uint32_t>mem;
    int s;
    memory(int size):mem(size){s=size;}
    void write(int index,uint32_t data){
        if(index<s&&index>=0)mem[index]=data;
    }
    uint32_t read(int index){
        if(index<s&&index>=0)return mem[index];
        return 0;
    }
};

class MO{
  public:
  class memory memoryunit;
  MO(int size):memoryunit(size){}
  void step(const EXMO&exmo,MOWB &next_mowb){
      if(!exmo.valid){
          next_mowb.valid=false;
          return;
      }
      if(exmo.CW.MemWrite)memoryunit.write((int)exmo.aluout,exmo.rs2);
      if(exmo.CW.MemRead)next_mowb.LDout=memoryunit.read((int)exmo.aluout);
      next_mowb.aluout=exmo.aluout;
      next_mowb.CW=exmo.CW;
      next_mowb.pc=exmo.pc;
      next_mowb.rdl=exmo.rdl;
      if(next_mowb.CW.jump||next_mowb.CW.jalr)next_mowb.aluout=exmo.pc+1;
      next_mowb.valid=true;
  }
};


class WB{
  public:
  void step(ID &id,const MOWB &mowb){
      if(!mowb.valid)return;
      if(mowb.CW.Regwrite){
          if(mowb.CW.Mem2reg){
              id.gpr.write(mowb.rdl,mowb.LDout);
          }else{
              id.gpr.write(mowb.rdl,mowb.aluout);
          }
          id.gpr.unlock(mowb.rdl,mowb.pc);
      }
  }
};
class pipeline{
    public:
    PCR pcr;
    IFID ifid;
    IDEX idex;
    EXMO exmo;
    MOWB mowb;
    IF If;
    ID id;
    EX ex;
    MO mo;
    WB wb;
    int cycle =1;
    pipeline(int memsize):mo(memsize){}
    void loadIM(string filename){
    //string filename = "sumofarray.txt";
    // string filename = "factorial.txt";
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
           uint32_t insnum=stringtonum(machine_code);
           If.IM.write(insnum,machine_code);
        }
    }
    
    infile.close();
    
}
void step()
{
    const int MAX_CYCLES=2000000;
    int safety=0;
    while(pcr.valid||ifid.valid||idex.valid||exmo.valid||mowb.valid)
    {
        if(++safety>MAX_CYCLES)
        {
            cerr<<"pipeline:reached MAX_CYCLES\n";
            break;
        }
        PCR next_pcr=pcr;
        next_pcr.pcchanged=false;
        IFID next_ifid{};
        IDEX next_idex{};
        EXMO next_exmo{};
        MOWB next_mowb{};
        wb.step(id,mowb);
        mo.step(exmo,next_mowb);
        bool stall_requested=false;
        ex.step(If,next_pcr,next_ifid,idex,next_idex,exmo,next_exmo,mowb,stall_requested);
        
    if(stall_requested)
    {
        next_ifid=ifid;
        next_idex=idex;
        next_pcr=pcr;
        next_pcr.pcchanged=false;
    }
    else{
        if(!next_exmo.flush)
        {
            id.step(ifid,next_idex);
        }
    }
    if(!next_exmo.flush && !stall_requested)
    {
       If.step(pcr,next_pcr,next_ifid);
    }
    else{
        
    }
    pcr=next_pcr;
    ifid=next_ifid;
    idex=next_idex;
    exmo=next_exmo;
    mowb=next_mowb;
    if(!idex.valid)
    {
        idex.opcode=idex.func3=idex.func7=idex.rdl=idex.rs1=idex.rs2=0;
    }
    if(!exmo.valid)
    {
        exmo.aluout=0;
        exmo.rdl=0;
        exmo.rs2=0;
    }
    if(!mowb.valid)
    {
        mowb.aluout=0;
        mowb.LDout=0;
        mowb.rdl=0;
    }
    cycle++;
}}
void status()
{
    cout<<"cycles"<<cycle<<endl;
    for(int i=0;i<32;i++)
      cout<<"x"<<i<<"="<<(int32_t)id.gpr.read(i)<<endl;
      for(int i=0;i<mo.memoryunit.s;i++)
      {
          cout<<"mem["<<i<<"]="<<mo.memoryunit.read(i)<<endl;
      }
}
};
int main()
{
pipeline abc(64);
abc.loadIM("input.txt");
abc.step();
abc.status();
return 0;
}













































