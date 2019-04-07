`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    09:55:16 10/26/2012 
// Design Name: 
// Module Name:    pipeidcu 
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
module pipeidcu(rsrtequ,func,
	             op,wreg,m2reg,wmem,aluc,regrt,aluimm,
					 sext,pcsource,shift,jal,
					 rs,rt,rd,
					 EXE_rd,EXE_wreg,MEM_rd,MEM_wreg,
					 ADEPEEN,BDEPEEN,EXE_SLD,LOADDEPEEN,
					 BTAKEN////////////////////
    );
	 input rsrtequ; 
	 input [5:0] func,op;
	 output wreg,m2reg,wmem,regrt,aluimm,sext,shift,jal;
	 input EXE_rd,EXE_wreg,MEM_rd,MEM_wreg;////////////
	 output [4:0] aluc;
	 output [1:0] pcsource;


	 input [4:0] rs,rt,rd;//////////////////
	 output [1:0] ADEPEEN,BDEPEEN;/////////////////
	 input EXE_SLD;//////////////////
	 output LOADDEPEEN;//////////////
	 wire ID_rs1IsReg,ID_rs2IsReg;///////////////////
	 wire ID_isIsStore;////////////////////
	 wire LOAD_EXE_A_DEPEN,LOAD_EXE_B_DEPEEN;////////////////
	 output BTAKEN;
     wire _wreg,_wmem;

	 wire i_add,i_sub,i_mul,i_and,i_or,i_xor,i_sll,i_srl,i_sra,i_jr;            //��ָ���������
	 wire i_addi,i_muli,i_andi,i_ori,i_xori,i_lw,i_sw,i_beq,i_bne,i_lui,i_j,i_jal;
	 
	 and(i_add,~op[5],~op[4],~op[3],~op[2],~op[1],~op[0],~func[2],~func[1],func[0]);
	 and(i_sub,~op[5],~op[4],~op[3],~op[2],~op[1],~op[0],~func[2],func[1],~func[0]);
	 and(i_mul,~op[5],~op[4],~op[3],~op[2],~op[1],~op[0],~func[2],func[1],func[0]);
	
	 
	 and(i_and,~op[5],~op[4],~op[3],~op[2],~op[1],op[0],~func[2],~func[1],func[0]);
	 and(i_or,~op[5],~op[4],~op[3],~op[2],~op[1],op[0],~func[2],func[1],~func[0]);
	
	 and(i_xor,~op[5],~op[4],~op[3],~op[2],~op[1],op[0],func[2],~func[1],~func[0]);
	 
	 and(i_sra,~op[5],~op[4],~op[3],~op[2],op[1],~op[0],~func[2],~func[1],func[0]);
	 and(i_srl,~op[5],~op[4],~op[3],~op[2],op[1],~op[0],~func[2],func[1],~func[0]);
	 and(i_sll,~op[5],~op[4],~op[3],~op[2],op[1],~op[0],~func[2],func[1],func[0]);
	 and(i_jr,~op[5],~op[4],~op[3],~op[2],op[1],~op[0],func[2],~func[1],~func[0]);
	 
	 and(i_addi,~op[5],~op[4],~op[3],op[2],~op[1],op[0]);
	
	 and(i_muli,~op[5],~op[4],~op[3],op[2],op[1],op[0]);
	 
	 
	 and(i_andi,~op[5],~op[4],op[3],~op[2],~op[1],op[0]);
	 and(i_ori,~op[5],~op[4],op[3],~op[2],op[1],~op[0]);

	 and(i_xori,~op[5],~op[4],op[3],op[2],~op[1],~op[0]);
	 
	 and(i_lw,~op[5],~op[4],op[3],op[2],~op[1],op[0]);
	 and(i_sw,~op[5],~op[4],op[3],op[2],op[1],~op[0]);
	 and(i_beq,~op[5],~op[4],op[3],op[2],op[1],op[0]);
	 and(i_bne,~op[5],op[4],~op[3],~op[2],~op[1],~op[0]);
	 and(i_lui,~op[5],op[4],~op[3],~op[2],~op[1],op[0]);
	 and(i_j,~op[5],op[4],~op[3],~op[2],op[1],~op[0]);
	 and(i_jal,~op[5],op[4],~op[3],~op[2],op[1],op[0]);
	
	 wire i_rs=i_add|i_sub|i_mul|i_and|i_or|i_xor|i_jr|i_addi|i_muli|           
	           i_andi|i_ori|i_xori|i_lw|i_sw|i_beq|i_bne;
	 wire i_rt=i_add|i_sub|i_mul|i_and|i_or|i_xor|i_sra|i_srl|i_sll|i_sw|i_beq|i_bne;
	 
    ////////////////////////////////////////////�����źŵ�����/////////////////////////////////////////////////////////
    assign _wreg=i_add|i_sub|i_mul|i_and|i_or|i_xor|i_sll|           //wregΪ1ʱд�Ĵ�������ĳһ�Ĵ���������д
	              i_srl|i_sra|i_addi|i_muli|i_andi|i_ori|i_xori|
					  i_lw|i_lui|i_jal;
	 assign regrt=i_addi|i_muli|i_andi|i_ori|i_xori|i_lw|i_lui;    //regrtΪ1ʱĿ�ļĴ�����rt������Ϊrd
	 assign jal=i_jal;                                           //Ϊ1ʱִ��jalָ�������
	 assign m2reg=i_lw;  //Ϊ1ʱ���洢������д��Ĵ���������ALU���д��Ĵ���
	 assign shift=i_sll|i_srl|i_sra;//Ϊ1ʱALUa�����ʹ����λλ��
	 assign aluimm=i_addi|i_muli|i_andi|i_ori|i_xori|i_lw|i_lui|i_sw;//Ϊ1ʱALUb�����ʹ��������
	 assign sext=i_addi|i_muli|i_lw|i_sw|i_beq|i_bne;//Ϊ1ʱ������չ����������չ
	 assign aluc[4]=i_sra;//ALU�Ŀ�����
	 assign aluc[3]=i_sub|i_or|i_ori|i_xor|i_xori| i_srl|i_sra|i_beq|i_bne;//ALU�Ŀ�����
	 assign aluc[2]=i_sll|i_srl|i_sra|i_lui;//ALU�Ŀ�����
	 assign aluc[1]=i_and|i_andi|i_or|i_ori|i_xor|i_xori|i_beq|i_bne;//ALU�Ŀ�����
	 assign aluc[0]=i_mul|i_muli|i_xor|i_xori|i_sll|i_srl|i_sra|i_beq|i_bne;//ALU�Ŀ�����

	 assign _wmem=i_sw;//Ϊ1ʱд�洢��������д
	 assign pcsource[1]=i_jr|i_j|i_jal;//ѡ����һ��ָ��ĵ�ַ��00ѡPC+4,01ѡת�Ƶ�ַ��10ѡ�Ĵ����ڵ�ַ��11ѡ��ת��ַ
	 assign pcsource[0]=i_beq&rsrtequ|i_bne&~rsrtequ|i_j|i_jal;
	




	///////////////////////////////////////////
	 // rs1 �Ƿ��ʾԴ�Ĵ���
	 assign ID_rs1IsReg = i_and | i_andi | i_or | i_ori | i_add | i_addi | i_sub  | i_lw | i_sw;//!!!!!!!!!!!!!!!!!!!!!!!!!!!
	 // rs2 �Ƿ�ΪԴ�Ĵ���
	 assign ID_rs2IsReg = i_and | i_or | i_add | i_sub;
	 // ����ָ���Ƿ�Ϊstore
	 assign ID_isIsStore = i_sw;
	 wire EXE_A_DEPEN, EXE_B_DEPEN, MEM_A_DEPEN, MEM_B_DEPEN, STALL;
	 // rs1 �� EXE�� rd �������
	 assign EXE_A_DEPEN = (rs==EXE_rd) && (EXE_wreg==1'b1) && (ID_rs1IsReg==1'b1);
	 	 // rs1 �� MEM�� rd �������
	 assign MEM_A_DEPEN = (rs==MEM_rd) && (MEM_wreg==1'b1) && (ID_rs1IsReg==1'b1);
	 
	 // rs2 �� EXE�� rd �������
	 assign EXE_B_DEPEN = ((rt==EXE_rd) && (EXE_wreg==1'b1) && (ID_rs2IsReg==1'b1))||
	 					  ((rd==EXE_rd)&&(EXE_wreg==1'b1)&&(ID_isIsStore==1'b1)); 
	 // rs2 �� MEM�� rd �������
	 assign MEM_B_DEPEN = ((rt==MEM_rd) && (MEM_wreg==1'b1) && (ID_rs2IsReg==1'b1))||
	 					   ((rd==MEM_rd)&&(MEM_wreg==1'b1)&&(ID_isIsStore==1'b1));
	 assign ADEPEEN[0] =  ((EXE_wreg == 1'b0) && (MEM_wreg == 1'b1) && (rs == MEM_rd)) ||
	 					  ((EXE_wreg == 1'b1) && (EXE_rd!=rs) && (MEM_wreg == 1'b1) && (rs == MEM_rd));
	 assign ADEPEEN[1] = (EXE_wreg == 1'b1 && EXE_rd==rs && MEM_wreg == 1'b0) ||
	 					 (EXE_wreg == 1'b0 && MEM_wreg == 1'b1 && rs == MEM_rd) ||
						 (EXE_wreg == 1'b1 && EXE_rd==rs && MEM_wreg == 1'b1 && rs != MEM_rd) ||
						 (EXE_wreg == 1'b1 && EXE_rd!=rs && MEM_wreg == 1'b1 && rs == MEM_rd) ||
						 (EXE_wreg == 1'b1 && EXE_rd==rs && MEM_wreg == 1'b1 && rs == MEM_rd);
	 assign BDEPEEN[0] = ID_rs2IsReg==1'b0 || ID_rs2IsReg==1'b1 && EXE_wreg == 1'b0 && MEM_wreg == 1'b1 && rt == MEM_rd ||
						ID_rs2IsReg==1'b1 && EXE_wreg == 1'b1 && EXE_rd !=rt && MEM_wreg == 1'b1 && rt == MEM_rd;
	 assign BDEPEEN[1] = ID_rs2IsReg==1'b1 && EXE_wreg == 1'b1 && EXE_rd ==rt && MEM_wreg == 1'b0||
						ID_rs2IsReg==1'b1 && EXE_wreg == 1'b0 && MEM_wreg == 1'b1 && rt == MEM_rd ||
						ID_rs2IsReg==1'b1 && EXE_wreg == 1'b1 && EXE_rd ==rt && MEM_wreg == 1'b1 && rt != MEM_rd ||
						ID_rs2IsReg==1'b1 && EXE_wreg == 1'b1 && EXE_rd !=rt && MEM_wreg == 1'b1 && rt == MEM_rd ||
						ID_rs2IsReg==1'b1 && EXE_wreg == 1'b1 && EXE_rd ==rt && MEM_wreg == 1'b1 && rt == MEM_rd;
	
	// load ��� 0 ��Ч
	assign LOADDEPEEN = ~(LOAD_EXE_A_DEPEN | LOAD_EXE_B_DEPEEN);
	// rs1 �� ִ�м�����load���
	assign LOAD_EXE_A_DEPEN = (rs==EXE_rd)&&(EXE_SLD==1'b1)&&(ID_rs1IsReg==1'b1);
	// rs2 ��ִ�м�����load���
	assign LOAD_EXE_B_DEPEEN =( (rt==EXE_rd)&&(EXE_SLD==1'b1)&&(ID_rs2IsReg==1'b1) )||
								(rd==EXE_rd)&&(EXE_SLD==1'b1)&&(ID_isIsStore == 1'b1);

	// _wreg��_wmem Ϊԭ������õ��Ŀ����ź�
	// ��load���ʱ��ִͣ�У���ֹд�Ĵ����ʹ洢��
	assign wreg = _wreg & LOADDEPEEN;
	assign wmem = _wmem & LOADDEPEEN;

	// �Ƿ���ת��ָ��
	assign BTAKEN= i_beq|i_bne|i_j;

endmodule
