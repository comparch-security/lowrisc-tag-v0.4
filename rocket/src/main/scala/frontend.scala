package rocket

import Chisel._
import uncore._
import Util._
import cde.{Parameters, Field}
import scala.math.max

class FrontendReq(implicit p: Parameters) extends CoreBundle()(p) {
  val pc = UInt(width = vaddrBitsExtended)
  val pc_tag = UInt(width = tgBits)
}

class FrontendResp(implicit p: Parameters) extends CoreBundle()(p) {
  val pc = UInt(width = vaddrBitsExtended)  // ID stage PC
  val pc_tag = UInt(width = tgBits)
  val data = Vec(fetchWidth, Bits(width = coreInstBits))
  val tag = Vec(fetchWidth, Bits(width = tgInstBits))
  val mask = Bits(width = fetchWidth)
  val xcpt_if = Bool()
}

class FrontendIO(implicit p: Parameters) extends CoreBundle()(p) {
  val req = Valid(new FrontendReq)
  val resp = Decoupled(new FrontendResp).flip
  val btb_resp = Valid(new BTBResp).flip
  val btb_update = Valid(new BTBUpdate)
  val bht_update = Valid(new BHTUpdate)
  val ras_update = Valid(new RASUpdate)
  val flush_icache = Bool(OUTPUT)
  val flush_tlb = Bool(OUTPUT)
  val npc = UInt(INPUT, width = vaddrBitsExtended)
}

class Frontend(implicit p: Parameters) extends CoreModule()(p) with HasL1CacheParameters {
  val io = new Bundle {
    val cpu = new FrontendIO().flip
    val ptw = new TLBPTWIO()
    val mem = new ClientUncachedTileLinkIO
    val pfc = new L1ICachePerform().flip
  }

  val icache = Module(new ICache)
  val tlb = Module(new TLB)

  val s1_pc_ = Reg(UInt(width=vaddrBitsExtended))
  val s1_pc = ~(~s1_pc_ | (coreInstBytes-1)) // discard PC LSBS (this propagates down the pipeline)
  val s1_pc_tag = Reg(init=UInt(0, tgBits))
  val s1_same_block = Reg(Bool())
  val s2_valid = Reg(init=Bool(true))
  val s2_pc = Reg(init=UInt(p(ResetVector)))
  val s2_pc_tag = Reg(init=UInt(0, tgBits))
  val s2_btb_resp_valid = Reg(init=Bool(false))
  val s2_btb_resp_bits = Reg(new BTBResp)
  val s2_xcpt_if = Reg(init=Bool(false))
  val s2_resp_valid = Wire(init=Bool(false))
  val s2_resp_data = Wire(UInt(width = rowBits))
  val s2_resp_tag = Wire(UInt(width = rowTagBits))

  val ntpc_0 = ~(~s1_pc | (coreInstBytes*fetchWidth-1)) + UInt(coreInstBytes*fetchWidth)
  val ntpc = // don't increment PC into virtual address space hole
    if (vaddrBitsExtended == vaddrBits) ntpc_0
    else Cat(s1_pc(vaddrBits-1) & ntpc_0(vaddrBits-1), ntpc_0)
  val predicted_npc = Wire(init = ntpc)
  val icmiss = s2_valid && !s2_resp_valid
  val npc = Mux(icmiss, s2_pc, predicted_npc).toUInt
  val npc_tag = Mux(icmiss, s2_pc_tag, UInt(0))
  val s0_same_block = Wire(init = !icmiss && !io.cpu.req.valid && ((ntpc & rowBytes) === (s1_pc & rowBytes)))

  val stall = io.cpu.resp.valid && !io.cpu.resp.ready
  when (!stall) {
    s1_same_block := s0_same_block && !tlb.io.resp.miss
    s1_pc_ := npc
    s1_pc_tag := npc_tag
    s2_valid := !icmiss
    when (!icmiss) {
      s2_pc := s1_pc
      s2_pc_tag := s1_pc_tag
      s2_xcpt_if := tlb.io.resp.xcpt_if
    }
  }
  when (io.cpu.req.valid) {
    s1_same_block := Bool(false)
    s1_pc_ := io.cpu.req.bits.pc
    s1_pc_tag := io.cpu.req.bits.pc_tag
    s2_valid := Bool(false)
  }

  if (p(BtbKey).nEntries > 0) {
    val btb = Module(new BTB)
    btb.io.req.valid := false
    btb.io.req.bits.addr := s1_pc
    btb.io.btb_update := io.cpu.btb_update
    btb.io.bht_update := io.cpu.bht_update
    btb.io.ras_update := io.cpu.ras_update
    btb.io.invalidate := io.cpu.flush_icache || io.cpu.flush_tlb // virtual tags
    when (!stall && !icmiss) {
      btb.io.req.valid := true
      s2_btb_resp_valid := btb.io.resp.valid
      s2_btb_resp_bits := btb.io.resp.bits
    }
    when (btb.io.resp.bits.taken) {
      predicted_npc := btb.io.resp.bits.target.sextTo(vaddrBitsExtended)
      s0_same_block := Bool(false)
    }
  }

  io.ptw <> tlb.io.ptw
  tlb.io.req.valid := !stall && !icmiss
  tlb.io.req.bits.vpn := s1_pc >> pgIdxBits
  tlb.io.req.bits.asid := UInt(0)
  tlb.io.req.bits.passthrough := Bool(false)
  tlb.io.req.bits.instruction := Bool(true)
  tlb.io.req.bits.store := Bool(false)

  io.mem <> icache.io.mem
  icache.io.req.valid := !stall && !s0_same_block
  icache.io.req.bits.idx := io.cpu.npc
  icache.io.invalidate := io.cpu.flush_icache
  icache.io.s1_ppn := tlb.io.resp.ppn
  icache.io.s1_kill := io.cpu.req.valid || tlb.io.resp.miss || tlb.io.resp.xcpt_if || icmiss || io.cpu.flush_tlb

  io.cpu.resp.valid := s2_valid && (s2_xcpt_if || s2_resp_valid)
  io.cpu.resp.bits.pc := s2_pc
  io.cpu.resp.bits.pc_tag := s2_pc_tag
  io.cpu.npc := Mux(io.cpu.req.valid, io.cpu.req.bits.pc, npc)

  // if the ways are buffered, we don't need to buffer again
  if (p(ICacheBufferWays)) {
    icache.io.resp.ready := !stall && !s1_same_block

    s2_resp_valid := icache.io.resp.valid
    s2_resp_data := icache.io.resp.bits.datablock
    s2_resp_tag := icache.io.resp.bits.tagblock
  } else {
    val icbuf = Module(new Queue(new ICacheResp, 1, pipe=true))
    icbuf.io.enq <> icache.io.resp
    icbuf.io.deq.ready := !stall && !s1_same_block

    s2_resp_valid := icbuf.io.deq.valid
    s2_resp_data := icbuf.io.deq.bits.datablock
    s2_resp_tag := icbuf.io.deq.bits.tagblock
  }

  require(fetchWidth * coreInstBytes <= rowBytes)
  val fetch_data =
    if (fetchWidth * coreInstBytes == rowBytes) s2_resp_data
    else s2_resp_data >> (s2_pc(log2Up(rowBytes)-1,log2Up(fetchWidth*coreInstBytes)) << log2Up(fetchWidth*coreInstBits))
  val fetch_tag =
    if (fetchWidth * coreInstBytes == rowBytes) s2_resp_tag
    else s2_resp_tag >> (s2_pc(log2Up(rowBytes)-1,2) * tgBits / 2)

  val coreInstTagBitsMax = max(coreInstBits, 32) / 32 * (tgBits/2)
  for (i <- 0 until fetchWidth) {
    io.cpu.resp.bits.data(i) := fetch_data(i*coreInstBits+coreInstBits-1, i*coreInstBits)
    io.cpu.resp.bits.tag(i) := fetch_tag(i*coreInstTagBitsMax+tgInstBits-1, i*coreInstTagBitsMax)
  }

  val all_ones = UInt((1 << (fetchWidth+1))-1)
  val msk_pc = if (fetchWidth == 1) all_ones else all_ones << s2_pc(log2Up(fetchWidth) -1+2,2)
  io.cpu.resp.bits.mask := msk_pc
  io.cpu.resp.bits.xcpt_if := s2_xcpt_if

  io.cpu.btb_resp.valid := s2_btb_resp_valid
  io.cpu.btb_resp.bits := s2_btb_resp_bits

  //L1IPFC
  io.pfc.read := Reg(next = io.cpu.resp.fire())
  io.pfc.read_miss := Reg(next = io.mem.acquire.fire())
}
