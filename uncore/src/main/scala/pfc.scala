// See LICENSE for license details.

package uncore

import Chisel._
import cde.{Parameters, Field}
import junctions._
import scala.math.{min, max}

case object PFCL2N extends Field[Int]

trait HasPFCParameters {
  implicit val p: Parameters
  val Tiles           = p(NTiles) //how many cores
  val Csrs            = Tiles
  val L1s             = Tiles
  val pfcTypes        = 3 //TilePFC L2PFC TCPFC
  val L2Banks         = p(PFCL2N) //Can't use p(NBanks)
  val TCBanks         = 1
  val Clients         = Csrs
  val ManagerIDs      = max(L1s,max(L2Banks,TCBanks))
  val NetPorts        = pfcTypes*(1<<log2Up(ManagerIDs))
  val MaxCounters     = 64
  //physical ID = Cat(types, ManagerID)
  val TilePFCfirstPID = 0  //PFCNetwork physical ID
  val L2PFCfirstPID   = 1<<log2Up(ManagerIDs)
  val TCPFCfirstPID   = 2<<log2Up(ManagerIDs)
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
  io.manager.resp.bits.last  := PopCount(bitmap) === UInt(1)
  io.manager.resp.bits.bitmapUI   := raddr(log2Up(nCounters)-1,0)
  io.manager.resp.bits.programID  := req_reg.programID

  when(io.manager.req.fire()) {
    req_reg   := io.manager.req.bits
    bitmap    := io.manager.req.bits.bitmap(nCounters-1,0)
    counterID := UInt(0)
  }
  when(io.manager.resp.fire()) {
    counterID := counterID+UInt(1)
    bitmap := rmleastO
  }

  when(state === s_IDLE && io.manager.req.fire()) {
    state := s_RESP
  }
  when(state === s_RESP && io.manager.resp.fire() && io.manager.resp.bits.last) {
    state := s_IDLE
  }
  when(req_reg.cmd(0)) { //finish or cancel
    state := s_IDLE
    req_reg.cmd           := UInt(0)
    io.manager.req.ready  := Bool(false)
    io.manager.resp.valid := Bool(false)
  }

}

class TilePFCManager(implicit p: Parameters) extends PFCModule()(p) {
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
}

class L2BankPFCManager(implicit p: Parameters) extends PFCModule()(p) {
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
}

class TCPFCManager(implicit p: Parameters) extends PFCModule()(p) {
  val io = new Bundle {
    val manager = new PFCManagerIO()
    val update  = new TagCachePerform()
  }

  val pfcManager = Module(new PFCManager(15))
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
}

class PFCCrossbar(implicit p: Parameters) extends PFCModule()(p) {
  val io  = new Bundle {
    val clients  = Vec(Clients, new PFCClientIO()).flip()
    val managers = Vec(NetPorts, new PFCManagerIO()).flip()
  }
  val reqNet  = Module(new BasicCrossbar(NetPorts, new PFCReq, count=2, Some((req: PhysicalNetworkIO[PFCReq]) => req.payload.hasMultibeatData())))
  val respNet = Module(new BasicCrossbar(NetPorts, new PFCResp, count=1, Some((resp: PhysicalNetworkIO[PFCResp]) => resp.payload.hasMultibeatData())))

  //csr <> Net
  (0 until Clients).map(i =>{
    //Csr.pfcreq to reqNet.in
    reqNet.io.in(i).valid             := io.clients(i).req.valid
    io.clients(i).req.ready           := reqNet.io.in(i).ready
    reqNet.io.in(i).bits.payload      := io.clients(i).req.bits
    reqNet.io.in(i).bits.header.src   := io.clients(i).req.bits.src
    reqNet.io.in(i).bits.header.dst   := io.clients(i).req.bits.dst
    //respNet.out to Csr.pfcresp
    io.clients(i).resp.valid          := respNet.io.out(i).valid
    respNet.io.out(i).ready           := io.clients(i).resp.ready
    io.clients(i).resp.bits           := respNet.io.out(i).bits.payload
  })

  //pfc <> Net
  (0 until NetPorts).map(i => {
    //reqNet.out to pfc.req
    io.managers(i).req.valid          := reqNet.io.out(i).valid
    reqNet.io.out(i).ready            := io.managers(i).req.ready
    io.managers(i).req.bits           := reqNet.io.out(i).bits.payload
    //pfc.resp to respNet.in
    respNet.io.in(i).valid            := io.managers(i).resp.valid
    io.managers(i).resp.ready         := respNet.io.in(i).ready
    respNet.io.in(i).bits.payload     := io.managers(i).resp.bits
    respNet.io.in(i).bits.header.src  := io.managers(i).resp.bits.src
    respNet.io.in(i).bits.header.dst  := io.managers(i).resp.bits.dst
  })
}
