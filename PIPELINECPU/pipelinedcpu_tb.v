`timescale 1ns / 1ps

////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer:
//
// Create Date:   17:13:23 10/29/2012
// Design Name:   pipelinedcpu
// Module Name:   D:/ORG_FPGA2012/PipelinedCpu/pipelinedcpu_tb.v
// Project Name:  PipelinedCpu
// Target Device:  
// Tool versions:  
// Description: 
//
// Verilog Test Fixture created by ISE for module: pipelinedcpu
//
// Dependencies:
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
////////////////////////////////////////////////////////////////////////////////

module pipelinedcpu_tb;

	// Inputs
	reg clock;
	reg resetn;

	// Outputs 
	wire [31:0] pc;
	wire [31:0] inst;
	wire [31:0] ealu;
	wire [31:0] malu;
	wire [31:0] walu;
	wire [1:0] TD_ADEPEEN;//////////
	wire [1:0] TD_BDEPEEN;/////////
	wire [1:0] TE_ADEPEEN;///////
	wire [1:0] TE_BDEPEEN;//////////
	wire TD_LOADDEPEEN;//////////
	wire TD_BTAKEN;//////////////
	wire [4:0]TE_RN, TW_RN;////////////////
	// Instantiate the Unit Under Test (UUT)
	pipelinedcpu uut (
		.clock(clock),  
		.resetn(resetn), 
		.pc(pc), 
		.inst(inst), 
		.ealu(ealu), 
		.malu(malu), 
		.walu(walu),
		.TD_ADEPEEN(TD_ADEPEEN),////////////
		.TD_BDEPEEN(TD_BDEPEEN),///////////////
		.TE_ADEPEEN(TE_ADEPEEN),////////////
		.TE_BDEPEEN(TE_BDEPEEN),////////////
		.TD_LOADDEPEEN(TD_LOADDEPEEN),////////////
		.TD_BTAKEN(TD_BTAKEN),////////////////
		.TE_RN(TE_RN),/////////////////////
		.TW_RN(TW_RN)////////////
	);

	initial begin
		// Initialize Inputs
		clock = 1;
		resetn = 0;

		// Wait 100 ns for global reset to finish
		#100;
		resetn = 1;
        
		// Add stimulus here

	end
	always #50 clock=~clock;
      
endmodule

