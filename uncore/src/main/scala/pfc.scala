// See LICENSE for license details.

package uncore

import Chisel._
import cde.{Parameters, Field}
import junctions._
import scala.math.{min, max}

trait HasPFCParameters {
  implicit val p: Parameters
  val PFCEmitLog      = true
  val Tiles           = p(NTiles) //how many cores
  val Csrs            = Tiles
  val L1s             = Tiles
  val pfcTypes        = 3 //TilePFC L2PFC TCPFC
  val L2Banks         = if(p(UseL2Cache)) p(NBanks) else 0
  val TCBanks         = if(p(UseTagMem)) 1 else 0
  val Clients         = Csrs
  val ManagerIDsWidth = log2Up(max(L1s,max(L2Banks,TCBanks)))
  val ManagerIDs      = 1<<ManagerIDsWidth
  val NetPorts        = pfcTypes*ManagerIDs
  val MaxCounters     = 64
  //physical ID = Cat(types, ManagerID)
  val TilePFCfirstPID = 0  //PFCNetwork physical ID
  val L2PFCfirstPID   = 1<<ManagerIDsWidth
  val TCPFCfirstPID   = 2<<ManagerIDsWidth
}

abstract class PFCModule(implicit val p: Parameters) extends Module with HasPFCParameters
abstract class PFCBundle(implicit val p: Parameters) extends ParameterizedBundle()(p) with HasPFCParameters

object PerFormanceCounter {
  def apply(op: UInt, n: Int): UInt = {
    val counter=Reg(init=UInt(0, n))
    counter := counter + op
    counter
  }
}


class L1ICachePerform extends Bundle {
  val read = Bool(INPUT)
  val read_miss = Bool(INPUT)
}

class L1DCachePerform extends Bundle {
  val read = Bool(INPUT)
  val read_miss = Bool(INPUT)
  val write =Bool(INPUT)
  val write_miss = Bool(INPUT)
  val write_back = Bool(INPUT)
}

class L2CachePerform extends Bundle {
  val read = Bool(INPUT) //inner.acquire
  val read_miss = Bool(INPUT) //outer.acquire
  val write = UInt(dir=INPUT, width=2) //
  //val write_miss =Bool(INPUT) //
  val write_back = UInt(dir=INPUT, width=2) //
}

class TagCachePerform extends Bundle {
  val readTT           = Bool(INPUT)
  val readTT_miss      = Bool(INPUT)
  val writeTT          = Bool(INPUT)
  val writeTT_miss     = Bool(INPUT)
  val writeTT_back     = Bool(INPUT)
  val readTM0          = Bool(INPUT)
  val readTM0_miss     = Bool(INPUT)
  val writeTM0         = Bool(INPUT)
  val writeTM0_miss    = Bool(INPUT)
  val writeTM0_back    = Bool(INPUT)
  val readTM1          = Bool(INPUT)
  val readTM1_miss     = Bool(INPUT)
  val writeTM1         = Bool(INPUT)
  val writeTM1_miss    = Bool(INPUT)
  val writeTM1_back    = Bool(INPUT)
  val acqTTfromMem     = Bool(INPUT) //acquire channel get tag table from mem
  val acqTM0fromMem    = Bool(INPUT) //acquire channel get tag map0  from mem
  val acqTM1fromMem    = Bool(INPUT) //acquire channel get tag map1  from mem
  val acqTfromMemT     = Bool(INPUT) //acquire channel get tag from mem total
  val acqTTtoMem       = Bool(INPUT) //acquire channel put tag table to mem
  val acqTM0toMem      = Bool(INPUT) //acquire channel put tag map0 to mem
  val acqTM1toMem      = Bool(INPUT) //acquire channel put tag map1 to mem
  val acqTtoMemT       = Bool(INPUT) //acquire channel put tag to mem total
  val serveTT          = UInt(INPUT, width=2) // mem xacts served by TT
  val serveTM0         = UInt(INPUT, width=2) // mem xacts served by TM0
  val serveTM1         = UInt(INPUT, width=2) // mem xacts served by TM1
  val accessTC         = UInt(INPUT, width=2) // number of TagTracker xacts
}

class TileCachePerformIO extends Bundle {
  val L1I = new L1ICachePerform()
  val L1D = new L1DCachePerform()
}

class TilePerform extends Bundle {
  val L1I = new L1ICachePerform()
  val L1D = new L1DCachePerform()
}

class PFCReq(implicit p: Parameters) extends PFCBundle()(p) {
 val src      = UInt(width=log2Up(Clients))
 val dst      = UInt(width=log2Up(NetPorts)) //groupID
 val cmd        = Bits(width=4)   //UInt(1) finish cancel
 val bitmap     = Bits(width=64)
 val programID  = Bits(width=4)
 def hasMultibeatData(dummy: Int = 0): Bool = Bool(true)
}

class PFCResp(implicit p: Parameters) extends PFCBundle()(p) {
  val src     = UInt(width=log2Up(NetPorts)) //groupID
  val dst     = UInt(width=log2Up(Clients))
  val first   = Bool() //indicate the first resp beat
  val last    = Bool() //indicate the last resp beat
  val data    = UInt(width=64) //pfc_data
  val bitmapUI   = UInt(width=6) //bit map UInt
  val programID  = Bits(width=4)
  def hasMultibeatData(dummy: Int = 0): Bool = Bool(true)
}

class PFCClientIO(implicit p: Parameters) extends PFCBundle()(p) {
  val req = Decoupled(new PFCReq())
  val resp = Decoupled(new PFCResp()).flip()
}

class PFCManagerIO(implicit p: Parameters) extends PFCBundle()(p) {
  val req = Decoupled(new PFCReq()).flip()
  val resp = Decoupled(new PFCResp())
}

class PFCManager(nCounters: Int)(implicit p: Parameters) extends PFCModule()(p) {
  val io = new Bundle {
    val manager = new PFCManagerIO()
    val update = Vec(nCounters, UInt(width=2, dir=INPUT))
  }

  val s_IDLE :: s_RESP :: Nil = Enum(UInt(), 2)
  val req_reg = Reg(new PFCReq())
  val firstresp = Reg(Bool())
  val state = Reg(init = s_IDLE)
  val counterID     = Reg(UInt(width=log2Up(nCounters)))
  val pfcounters    = Vec(nCounters, Wire(UInt(width=64)))
  val bitmap        = Reg(UInt(width=nCounters))
  val rmleastO      = Wire(UInt(width=nCounters)) //remove least one in bitmap
  val raddr         = Wire(UInt(width=log2Up(nCounters)+1))
  /**e.g. 0001=>0  0110=>1  1000=>3  0000=>4 */
  raddr    := PriorityMux(Cat(UInt(0), bitmap), (0 until nCounters+1).map(UInt(_)))
  /**e.g. 0101=>0100  0110=>0100  1000=>0000  0000=>0000 */
  if(nCounters==1) rmleastO := UInt(0)
  else rmleastO := bitmap & (UInt((1<<nCounters)-2) << raddr(log2Up(nCounters)-1,0))

  (0 until nCounters).map(i => {
    pfcounters(i) := PerFormanceCounter(io.update(i), 64)
  })

  io.manager.req.ready       := Bool(true)
  io.manager.resp.valid      := state === s_RESP
  io.manager.resp.bits.dst   := req_reg.src
  io.manager.resp.bits.data  := pfcounters(raddr)
  io.manager.resp.bits.first := firstresp
  io.manager.resp.bits.last  := PopCount(bitmap) <= UInt(1)
  io.manager.resp.bits.bitmapUI   := raddr(log2Up(nCounters)-1,0)
  io.manager.resp.bits.programID  := req_reg.programID

  when(io.manager.req.fire()) {
    req_reg   := io.manager.req.bits
    bitmap    := io.manager.req.bits.bitmap(nCounters-1,0)
    counterID := UInt(0)
    firstresp := Bool(true)
  }
  when(io.manager.resp.fire()) {
    counterID := counterID+UInt(1)
    bitmap := rmleastO
    firstresp := Bool(false)
  }

  when(io.manager.req.fire()) {
    //cmd===UInt(0) csr require a new pfc, otherwise cancel or finish pfc
    state := Mux(io.manager.req.bits.cmd===UInt(0), s_RESP, s_IDLE)
  }
  when(state === s_RESP && io.manager.resp.fire() && io.manager.resp.bits.last) {
    state := s_IDLE
  }
}

class TilePFCManager(id: Int)(implicit p: Parameters) extends PFCModule()(p) {
  val io = new Bundle {
    val manager = new PFCManagerIO()
    val update  = new TilePerform()
  }

  val pfcManager = Module(new PFCManager(7))
  io.manager <> pfcManager.io.manager
  //L1I
  pfcManager.io.update(0) := io.update.L1I.read
  pfcManager.io.update(1) := io.update.L1I.read_miss
  //L1D
  pfcManager.io.update(2) := io.update.L1D.read
  pfcManager.io.update(3) := io.update.L1D.read_miss
  pfcManager.io.update(4) := io.update.L1D.write
  pfcManager.io.update(5) := io.update.L1D.write_miss
  pfcManager.io.update(6) := io.update.L1D.write_back

  if(PFCEmitLog) {
    val resp_pfc = io.manager.resp.bits.data
    val resp_bitmapUI = io.manager.resp.bits.bitmapUI
    when(io.manager.resp.fire()) {
      when(resp_bitmapUI===UInt(0)) { printf("PFCResp: Tile%d L1I_read = %d\n",        UInt(id), resp_pfc)}
      when(resp_bitmapUI===UInt(1)) { printf("PFCResp: Tile%d L1I_readmiss = %d\n",    UInt(id), resp_pfc)}
      when(resp_bitmapUI===UInt(2)) { printf("PFCResp: Tile%d L1D_read = %d\n",        UInt(id), resp_pfc)}
      when(resp_bitmapUI===UInt(3)) { printf("PFCResp: Tile%d L1D_readmiss = %d\n",    UInt(id), resp_pfc)}
      when(resp_bitmapUI===UInt(4)) { printf("PFCResp: Tile%d L1D_write = %d\n",       UInt(id), resp_pfc)}
      when(resp_bitmapUI===UInt(5)) { printf("PFCResp: Tile%d L1D_writemiss = %d\n",   UInt(id), resp_pfc)}
      when(resp_bitmapUI===UInt(6)) { printf("PFCResp: Tile%d L1D_writeback = %d\n",   UInt(id), resp_pfc)}
    }
  }
}

class L2BankPFCManager(id: Int)(implicit p: Parameters) extends PFCModule()(p) {
  val io = new Bundle {
    val manager = new PFCManagerIO()
    val update  = new L2CachePerform()
  }

  val pfcManager = Module(new PFCManager(4))
  io.manager <> pfcManager.io.manager
  pfcManager.io.update(0) := io.update.read
  pfcManager.io.update(1) := io.update.read_miss
  pfcManager.io.update(2) := io.update.write
  pfcManager.io.update(3) := io.update.write_back

  if(PFCEmitLog) {
    val resp_pfc = io.manager.resp.bits.data
    val resp_bitmapUI = io.manager.resp.bits.bitmapUI
    when(io.manager.resp.fire()) {
      when(resp_bitmapUI===UInt(0)) { printf("PFCResp: L2Bank%d read = %d\n",        UInt(id), resp_pfc)}
      when(resp_bitmapUI===UInt(1)) { printf("PFCResp: L2Bank%d readmiss = %d\n",    UInt(id), resp_pfc)}
      when(resp_bitmapUI===UInt(2)) { printf("PFCResp: L2Bank%d write = %d\n",       UInt(id), resp_pfc)}
      when(resp_bitmapUI===UInt(3)) { printf("PFCResp: L2Bank%d writeback = %d\n",   UInt(id), resp_pfc)}
    }
  }
}

class TCPFCManager(implicit p: Parameters) extends PFCModule()(p) {
  val io = new Bundle {
    val manager = new PFCManagerIO()
    val update  = new TagCachePerform()
  }

  val pfcManager = Module(new PFCManager(27))
  io.manager <> pfcManager.io.manager
  pfcManager.io.update(0) := io.update.readTT
  pfcManager.io.update(1) := io.update.readTT_miss
  pfcManager.io.update(2) := io.update.writeTT
  pfcManager.io.update(3) := io.update.writeTT_miss
  pfcManager.io.update(4) := io.update.writeTT_back
  pfcManager.io.update(5) := io.update.readTM0
  pfcManager.io.update(6) := io.update.readTM0_miss
  pfcManager.io.update(7) := io.update.writeTM0
  pfcManager.io.update(8) := io.update.writeTM0_miss
  pfcManager.io.update(9) := io.update.writeTM0_back
  pfcManager.io.update(10) := io.update.readTM1
  pfcManager.io.update(11) := io.update.readTM1_miss
  pfcManager.io.update(12) := io.update.writeTM1
  pfcManager.io.update(13) := io.update.writeTM1_miss
  pfcManager.io.update(14) := io.update.writeTM1_back
  pfcManager.io.update(15) := io.update.acqTTfromMem
  pfcManager.io.update(16) := io.update.acqTM0fromMem
  pfcManager.io.update(17) := io.update.acqTM1fromMem
  pfcManager.io.update(18) := io.update.acqTfromMemT
  pfcManager.io.update(19) := io.update.acqTTtoMem
  pfcManager.io.update(20) := io.update.acqTM0toMem
  pfcManager.io.update(21) := io.update.acqTM1toMem
  pfcManager.io.update(22) := io.update.acqTtoMemT
  pfcManager.io.update(23) := io.update.serveTT
  pfcManager.io.update(24) := io.update.serveTM0
  pfcManager.io.update(25) := io.update.serveTM1
  pfcManager.io.update(26) := io.update.accessTC

  if(PFCEmitLog) {
    val resp_pfc = io.manager.resp.bits.data
    val resp_bitmapUI = io.manager.resp.bits.bitmapUI
    when(io.manager.resp.fire()) {
      when(resp_bitmapUI===UInt(0))  { printf("PFCResp: TC readTT = %d\n",        resp_pfc)}
      when(resp_bitmapUI===UInt(1))  { printf("PFCResp: TC readTTmiss = %d\n",    resp_pfc)}
      when(resp_bitmapUI===UInt(2))  { printf("PFCResp: TC writeTT = %d\n",       resp_pfc)}
      when(resp_bitmapUI===UInt(3))  { printf("PFCResp: TC writeTTmiss = %d\n",   resp_pfc)}
      when(resp_bitmapUI===UInt(4))  { printf("PFCResp: TC writeTTback = %d\n",   resp_pfc)}
      when(resp_bitmapUI===UInt(5))  { printf("PFCResp: TC readTM0 = %d\n",       resp_pfc)}
      when(resp_bitmapUI===UInt(6))  { printf("PFCResp: TC readTM0miss = %d\n",   resp_pfc)}
      when(resp_bitmapUI===UInt(7))  { printf("PFCResp: TC writeTM0 = %d\n",      resp_pfc)}
      when(resp_bitmapUI===UInt(8))  { printf("PFCResp: TC writeTM0miss = %d\n",  resp_pfc)}
      when(resp_bitmapUI===UInt(9))  { printf("PFCResp: TC writeTM0back = %d\n",  resp_pfc)}
      when(resp_bitmapUI===UInt(10)) { printf("PFCResp: TC readTM1 = %d\n",       resp_pfc)}
      when(resp_bitmapUI===UInt(11)) { printf("PFCResp: TC readTM1miss = %d\n",   resp_pfc)}
      when(resp_bitmapUI===UInt(12)) { printf("PFCResp: TC writeTM1 = %d\n",      resp_pfc)}
      when(resp_bitmapUI===UInt(13)) { printf("PFCResp: TC writeTM1miss = %d\n",  resp_pfc)}
      when(resp_bitmapUI===UInt(14)) { printf("PFCResp: TC writeTM1back = %d\n",  resp_pfc)}
      when(resp_bitmapUI===UInt(15)) { printf("PFCResp: TC acqTTfromMem = %d\n",  resp_pfc)}
      when(resp_bitmapUI===UInt(16)) { printf("PFCResp: TC acqTM0fromMem = %d\n", resp_pfc)}
      when(resp_bitmapUI===UInt(17)) { printf("PFCResp: TC acqTM1fromMem = %d\n", resp_pfc)}
      when(resp_bitmapUI===UInt(18)) { printf("PFCResp: TC acqTfromMemT = %d\n",  resp_pfc)}
      when(resp_bitmapUI===UInt(19)) { printf("PFCResp: TC acqTTtoMem = %d\n",    resp_pfc)}
      when(resp_bitmapUI===UInt(20)) { printf("PFCResp: TC acqTM0toMem = %d\n",   resp_pfc)}
      when(resp_bitmapUI===UInt(21)) { printf("PFCResp: TC acqTM1toMem = %d\n",   resp_pfc)}
      when(resp_bitmapUI===UInt(22)) { printf("PFCResp: TC acqTtoMemT = %d\n",    resp_pfc)}
      when(resp_bitmapUI===UInt(23)) { printf("PFCResp: TC serveTT = %d\n",       resp_pfc)}
      when(resp_bitmapUI===UInt(24)) { printf("PFCResp: TC serveTM0 = %d\n",      resp_pfc)}
      when(resp_bitmapUI===UInt(25)) { printf("PFCResp: TC serveTM1 = %d\n",      resp_pfc)}
      when(resp_bitmapUI===UInt(26)) { printf("PFCResp: TC accessTC = %d\n",      resp_pfc)}
    }
  }
}

class PFCCrossbar(implicit p: Parameters) extends PFCModule()(p) {
  val io  = new Bundle {
    val clients  = Vec(Clients, new PFCClientIO()).flip()
    val managers = Vec(NetPorts, new PFCManagerIO()).flip()
  }
  val reqNet  = Module(new BasicCrossbar(NetPorts, new PFCReq, count=2, Some((req: PhysicalNetworkIO[PFCReq]) => req.payload.hasMultibeatData())))
  val respNet = Module(new BasicCrossbar(NetPorts, new PFCResp, count=1, Some((resp: PhysicalNetworkIO[PFCResp]) => resp.payload.hasMultibeatData())))

  //csr <> Net
  (0 until NetPorts).map(i =>{
    //disable unused port
    if(i >= TilePFCfirstPID+Tiles) {
      reqNet.io.in(i).valid := Bool(false)
      respNet.io.out(i).ready := Bool(false)
    } else {
      //Csr.pfcreq to reqNet.in
      reqNet.io.in(i).valid := io.clients(i).req.valid
      io.clients(i).req.ready := reqNet.io.in(i).ready
      reqNet.io.in(i).bits.payload := io.clients(i).req.bits
      reqNet.io.in(i).bits.header.src := io.clients(i).req.bits.src
      reqNet.io.in(i).bits.header.dst := io.clients(i).req.bits.dst
      //respNet.out to Csr.pfcresp
      io.clients(i).resp.valid := respNet.io.out(i).valid
      respNet.io.out(i).ready := io.clients(i).resp.ready
      io.clients(i).resp.bits := respNet.io.out(i).bits.payload
    }
  })

  //pfc <> Net
  (0 until NetPorts).map(i => {
    //disable unused port
    if((i>=TilePFCfirstPID+Tiles && i<L2PFCfirstPID) ||
       (i>=L2PFCfirstPID+L2Banks && i<TCPFCfirstPID) ||
       (i>=TCPFCfirstPID+TCBanks)) {
      reqNet.io.out(i).ready := Bool(false)
      respNet.io.in(i).valid  := Bool(false)
    } else {
      //reqNet.out to pfc.req
      io.managers(i).req.valid := reqNet.io.out(i).valid
      reqNet.io.out(i).ready := io.managers(i).req.ready
      io.managers(i).req.bits := reqNet.io.out(i).bits.payload
      //pfc.resp to respNet.in
      respNet.io.in(i).valid := io.managers(i).resp.valid
      io.managers(i).resp.ready := respNet.io.in(i).ready
      respNet.io.in(i).bits.payload := io.managers(i).resp.bits
      respNet.io.in(i).bits.header.src := io.managers(i).resp.bits.src
      respNet.io.in(i).bits.header.dst := io.managers(i).resp.bits.dst
    }
  })
}
