`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    14:59:18 10/26/2012 
// Design Name: 
// Module Name:    pipedereg 
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
module pipedereg(dwreg,dm2reg,dwmem,daluc,daluimm,da,db,dimm,drn,
                 dshift,djal,dpc4,clk,clrn,ewreg,em2reg,ewmem,
					  ealuc,ealuimm,ea,eb,eimm,ern,eshift,ejal,epc4,
					  dADEPEEN,dBDEPEEN,eADEPEEN,eBDEPEEN,//////////////////////
					  dBTAKEN,eBTAKEN  //增加BTAKEN信号
    );
	 input [31:0] da,db,dimm,dpc4;
	 input [4:0] drn;
	 input [4:0] daluc;
	 input dwreg,dm2reg,dwmem,daluimm,dshift,djal;
	 input clk,clrn;
	 input [1:0] dADEPEEN,dBDEPEEN;
	 output [1:0] eADEPEEN,eBDEPEEN;
	 input dBTAKEN;///////////!!!
	 output eBTAKEN;//////!!!
	 output [31:0] ea,eb,eimm,epc4;
	 output [4:0] ern;
	 output [4:0] ealuc;
	 output ewreg,em2reg,ewmem,ealuimm,eshift,ejal;
	 reg [31:0] ea,eb,eimm,epc4;
	 reg [4:0] ern;
	 reg [4:0] ealuc;
	 reg ewreg,em2reg,ewmem,ealuimm,eshift,ejal;
	 reg [1:0] eADEPEEN,eBDEPEEN;//!!!!!!!!!!!!!!!!!
	 reg eBTAKEN;//!!!!!!!!!!!!
	 always @(negedge clrn or posedge clk)
	     if(clrn==0)
		      begin
				    ewreg<=0;    em2reg<=0;
					 ewmem<=0;    ealuc<=0;
					 ealuimm<=0;  ea<=0;
					 eb<=0;       eimm<=0;
					 ern<=0;      eshift<=0;
					 ejal<=0;     epc4<=0;
					 eADEPEEN<=2'b00; eBDEPEEN<=2'b00;//////////////////
					 eBTAKEN<=0;///////////////////////////////
			   end
			else
			    begin
				    ewreg<=dwreg;    em2reg<=dm2reg;
					 ewmem<=dwmem;    ealuc<=daluc;
					 ealuimm<=daluimm;  ea<=da;
					 eb<=db;       eimm<=dimm;
					 ern<=drn;      eshift<=dshift;
					 ejal<=djal;     epc4<=dpc4;
					 eADEPEEN<=dADEPEEN; eBDEPEEN<=dBDEPEEN;
					 eBTAKEN<=dBTAKEN;//传递BTAKEN信号
				 end

endmodule
