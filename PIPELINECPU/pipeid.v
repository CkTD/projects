`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    17:26:59 10/25/2012 
// Design Name: 
// Module Name:    pipeid 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module pipeid(dpc4,inst,wrn,
              wdi,wwreg,clk,clrn,bpc,jpc,pcsource,
				  wreg,m2reg,wmem,aluc,aluimm,a,b,imm,rn,
				  shift,jal,//////////////
				  EXE_rd,EXE_wreg,MEM_rd,MEM_wreg,ADEPEEN,BDEPEEN,
				  EXE_SLD,LOADDEPEEN,///////////
				  dBTAKEN,eBTAKEN///////////////////
    );
	 input [31:0] dpc4,inst,wdi;
	 input [4:0] wrn;
	 input wwreg;
	 input clk,clrn;

	 input EXE_rd,EXE_wreg,MEM_rd,MEM_wreg;	// 增加用于在ID级检测相关的输入信号
	 output [31:0] bpc,jpc,a,b,imm;
	 output [4:0] rn;
	 output [4:0] aluc;
	 output [1:0] pcsource;
	 output wreg,m2reg,wmem,aluimm,shift,jal;
	 output [1:0] ADEPEEN,BDEPEEN; // 输出两个操作数的来源，相关时实现FORWORDING
	 output dBTAKEN;/////
	 input eBTAKEN;////////
	 input EXE_SLD;//////////////////
	 output LOADDEPEEN;/////////////////
	 wire [5:0] op,func;
	 wire [4:0] rs,rt,rd;
	 wire [31:0] qa,qb,br_offset;
	 wire [15:0] ext16;
	 wire regrt,sext,e;
	 
	 wire rsrtequ;
	 wire _wreg,_wmem;	//cu直接产生的写信号

	// 直接比较rs和rt的值是否相同，得到rsrtequ
	 assign rsrtequ = (qa==qb)?1'b1:1'b0;
	 
	 assign func=inst[25:20];  
	 assign op=inst[31:26];
	 assign rs=inst[9:5];
	 assign rt=inst[4:0];
	 assign rd=inst[14:10];
	 assign jpc={dpc4[31:28],inst[25:0],2'b00};//jump,jal指令的目标地址的计算


	// 执行级是转移指令，则废弃本条指令。
	 assign wreg = _wreg & ~eBTAKEN;/////
	 assign wmem = _wmem & ~eBTAKEN;////

	 pipeidcu cu(rsrtequ,func,                          //控制部件
	             op,_wreg,m2reg,_wmem,aluc,regrt,aluimm,
					 sext,pcsource,shift,jal,rs,rt,rd,EXE_rd,EXE_wreg,MEM_rd,MEM_wreg,ADEPEEN,BDEPEEN,EXE_SLD,LOADDEPEEN,dBTAKEN);//////////////////
    regfile rf (rs,rt,wdi,wrn,wwreg,~clk,clrn,qa,qb);//寄存器堆，有32个32位的寄存器，0号寄存器恒为0
	 mux2x5 des_reg_no (rd,rt,regrt,rn); //选择目的寄存器是来自于rd,还是rt
	 assign a=qa;
	 assign b=qb;
	 assign e=sext&inst[25];//符号拓展或0拓展
	 assign ext16={16{e}};//符号拓展
	 assign imm={ext16,inst[25:10]};//将立即数进行符号拓展
	 assign br_offset={imm[29:0],2'b00};
	 cla32 br_addr (dpc4,br_offset,1'b0,bpc);//beq,bne指令的目标地址的计算
	
endmodule
