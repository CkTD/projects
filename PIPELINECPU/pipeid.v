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

	 input EXE_rd,EXE_wreg,MEM_rd,MEM_wreg;	// ����������ID�������ص������ź�
	 output [31:0] bpc,jpc,a,b,imm;
	 output [4:0] rn;
	 output [4:0] aluc;
	 output [1:0] pcsource;
	 output wreg,m2reg,wmem,aluimm,shift,jal;
	 output [1:0] ADEPEEN,BDEPEEN; // �����������������Դ�����ʱʵ��FORWORDING
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
	 wire _wreg,_wmem;	//cuֱ�Ӳ�����д�ź�

	// ֱ�ӱȽ�rs��rt��ֵ�Ƿ���ͬ���õ�rsrtequ
	 assign rsrtequ = (qa==qb)?1'b1:1'b0;
	 
	 assign func=inst[25:20];  
	 assign op=inst[31:26];
	 assign rs=inst[9:5];
	 assign rt=inst[4:0];
	 assign rd=inst[14:10];
	 assign jpc={dpc4[31:28],inst[25:0],2'b00};//jump,jalָ���Ŀ���ַ�ļ���


	// ִ�м���ת��ָ����������ָ�
	 assign wreg = _wreg & ~eBTAKEN;/////
	 assign wmem = _wmem & ~eBTAKEN;////

	 pipeidcu cu(rsrtequ,func,                          //���Ʋ���
	             op,_wreg,m2reg,_wmem,aluc,regrt,aluimm,
					 sext,pcsource,shift,jal,rs,rt,rd,EXE_rd,EXE_wreg,MEM_rd,MEM_wreg,ADEPEEN,BDEPEEN,EXE_SLD,LOADDEPEEN,dBTAKEN);//////////////////
    regfile rf (rs,rt,wdi,wrn,wwreg,~clk,clrn,qa,qb);//�Ĵ����ѣ���32��32λ�ļĴ�����0�żĴ�����Ϊ0
	 mux2x5 des_reg_no (rd,rt,regrt,rn); //ѡ��Ŀ�ļĴ�����������rd,����rt
	 assign a=qa;
	 assign b=qb;
	 assign e=sext&inst[25];//������չ��0��չ
	 assign ext16={16{e}};//������չ
	 assign imm={ext16,inst[25:10]};//�����������з�����չ
	 assign br_offset={imm[29:0],2'b00};
	 cla32 br_addr (dpc4,br_offset,1'b0,bpc);//beq,bneָ���Ŀ���ַ�ļ���
	
endmodule
