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
	 assign sa={eimm[4:0],eimm[31:5]};//ï¿½ï¿½Î»Î»ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	 cla32 ret_addr(epc4,32'h4,1'b0,epc8);//ï¿½ï¿½PC+4ï¿½Ù¼ï¿½4ï¿½ï¿½ï¿½PC+8ï¿½ï¿½ï¿½ï¿½jalï¿½ï¿½
	 //mux2x32 alu_ina (ea,sa,eshift,alua);//Ñ¡ï¿½ï¿½ALU aï¿½Ëµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ô´
	 //mux2x32 alu_inb (eb,eimm,ealuimm,alub);//Ñ¡ï¿½ï¿½ALU bï¿½Ëµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ô´
	 //mux2x32 alu_inaB (ea,sa,eshift,eaa);////////////////////////////////////////////////
	 mux4x32 alu_ina(ea,{32{0}},malu,walu,eADEPEEN,alua);//¸ù¾ÝADEPEENÑ¡ÔñµÚÒ»¸öÔ´²Ù×÷Êý
	 mux4x32 alu_inb(eb,eimm,malu,walu,eBDEPEEN,alub);//¸ù¾ÝBDEPEENÑ¡ÔñµÚ¶þ¸öÔ´²Ù×÷Êý
	 mux2x32 save_pc8(ealu0,epc8,ejal,ealu);//Ñ¡ï¿½ï¿½ï¿½ï¿½ï¿½ALUï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ô´ï¿½ï¿½ejalÎª0Ê±ï¿½ï¿½ALUï¿½Ú²ï¿½ï¿½ï¿½ï¿½ï¿½Ä½ï¿½ï¿½ï¿½ï¿½Îª1Ê±ï¿½ï¿½PC+8
	 assign ern=ern0|{5{ejal}};//ï¿½ï¿½jalÖ¸ï¿½ï¿½Ö´ï¿½ï¿½Ê±ï¿½ï¿½ï¿½Ñ·ï¿½ï¿½Øµï¿½Ö·Ð´ï¿½ï¿½31ï¿½Å¼Ä´ï¿½ï¿½ï¿½
	 alu al_unit (alua,alub,ealuc,ealu0,z);//ALU

endmodule
