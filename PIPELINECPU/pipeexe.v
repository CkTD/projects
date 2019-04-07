`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    15:18:25 10/26/2012 
// Design Name: 
// Module Name:    pipeexe 
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
module pipeexe(ealuc,ealuimm,ea,eb,eimm,eshift,ern0,epc4,ejal,ern,ealu,z,eADEPEEN,eBDEPEEN,malu,walu//////////////////
    );
	 input [31:0] ea,eb,eimm,epc4;
	 input [4:0] ern0;
	 input [4:0] ealuc;
	 input ealuimm,eshift,ejal;
	 input [31:0] malu, walu;
	 input [1:0]eADEPEEN,eBDEPEEN;///////////////////////////////////
	 output [31:0] ealu;
	 output [4:0] ern;
	 wire [31:0] alua,alub,sa,ealu0,epc8;
	 //wire eaa;
	 output z;
	 assign sa={eimm[4:0],eimm[31:5]};//��λλ��������
	 cla32 ret_addr(epc4,32'h4,1'b0,epc8);//��PC+4�ټ�4���PC+8����jal��
	 //mux2x32 alu_ina (ea,sa,eshift,alua);//ѡ��ALU a�˵�������Դ
	 //mux2x32 alu_inb (eb,eimm,ealuimm,alub);//ѡ��ALU b�˵�������Դ
	 //mux2x32 alu_inaB (ea,sa,eshift,eaa);////////////////////////////////////////////////
	 mux4x32 alu_ina(ea,{32{0}},malu,walu,eADEPEEN,alua);//����ADEPEENѡ���һ��Դ������
	 mux4x32 alu_inb(eb,eimm,malu,walu,eBDEPEEN,alub);//����BDEPEENѡ��ڶ���Դ������
	 mux2x32 save_pc8(ealu0,epc8,ejal,ealu);//ѡ�����ALU�������Դ��ejalΪ0ʱ��ALU�ڲ�����Ľ����Ϊ1ʱ��PC+8
	 assign ern=ern0|{5{ejal}};//��jalָ��ִ��ʱ���ѷ��ص�ַд��31�żĴ���
	 alu al_unit (alua,alub,ealuc,ealu0,z);//ALU

endmodule
