// See LICENSE for license details.

package uncore

import Chisel._
import cde.{Parameters, Field}
import junctions._

case object PFCL2N extends Field[Int]

trait HasPFCParameters {
  implicit val p: Parameters
  val Tiles     = p(NTiles) //how many cores
  val Csrs      = Tiles
  val L1s       = Tiles
  val L2Banks   = p(PFCL2N) //Can't use p(NBanks)
  val TCBanks   = 1
  val Clients   = Csrs
  val Managers  = L1s+L2Banks+TCBanks //pfc are clients
  val NetPorts  = if(Managers>Clients) Managers else Clients
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

class L2CachePerformCounter extends Bundle {
   val read       = UInt(width=64)
   val read_miss  = UInt(width=64)
   val write      = UInt(width=64)
   //val write_miss = UInt(width=64)
   val write_back = UInt(width=64)
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

class TagCachePerformCounter extends Bundle {
  val  readTT          = UInt(width=64)
  val  readTT_miss     = UInt(width=64)
  val  writeTT         = UInt(width=64)
  val  writeTT_miss    = UInt(width=64)
  val  writeTT_back    = UInt(width=64)
  val  readTM0         = UInt(width=64)
  val  readTM0_miss    = UInt(width=64)
  val  writeTM0        = UInt(width=64)
  val  writeTM0_miss   = UInt(width=64)
  val  writeTM0_back   = UInt(width=64)
  val  readTM1         = UInt(width=64)
  val  readTM1_miss    = UInt(width=64)
  val  writeTM1        = UInt(width=64)
  val  writeTM1_miss   = UInt(width=64)
  val  writeTM1_back   = UInt(width=64)
}

class PrivatePerform extends Bundle {
  val L1I = new L1ICachePerform()
  val L1D = new L1DCachePerform()
}

//class SharePerform(implicit val p: Parameters) extends PFCBundle { //WRONG
//  val L2D = Vec(L2DBanks, new L2DCachePerform)
class SharePerform(implicit p: Parameters) extends PFCBundle()(p) {
  val L2D = Vec(L2Banks, new L2CachePerform)
  val TAG = new TagCachePerform()
}

class PFCReq(implicit p: Parameters) extends PFCBundle()(p) {
 val src      = UInt(width=log2Up(NetPorts))
 val dst      = UInt(width=log2Up(NetPorts))
 val cmd      = Bits(width=2) //00:read 01:acquire
 val addr     = Bits(width=6)
 val groupID  = Bits(width=4) //0001:PrivatePFC 0010:SharePFC others:reserve
 val subGroID = Bits(width=4)
 //for PrivatePFC 0001:L1I 0010:L1D
 //for SharePFC   0001:L2  0010:TC
 def hasMultibeatData(dummy: Int = 0): Bool = Bool(false)
}

class PFCResp(implicit p: Parameters) extends PFCBundle()(p) {
  val src     = UInt(width=log2Up(NetPorts))
  val dst     = UInt(width=log2Up(NetPorts))
  val data    = UInt(width=64)
  def hasMultibeatData(dummy: Int = 0): Bool = Bool(true)
}

class PrivatePFC(implicit p: Parameters) extends PFCModule()(p) {
  val io = new Bundle {
    val req = Valid(new PFCReq()).flip() //req from csr
    val resp = Valid(new PFCResp()) //resp to csr
    val update = new PrivatePerform()
  }

  val groupMatch = io.req.valid && io.req.bits.groupID === UInt(1) //4'b0001
  val groupisL1I = groupMatch && io.req.bits.subGroID === UInt(1) //4'b0001
  val groupisL1D = groupMatch && io.req.bits.subGroID === UInt(2) //4'b0010
  val read = groupMatch && io.req.bits.cmd === UInt(0)
  val acquire = groupMatch && io.req.bits.cmd === UInt(1)

  //import rocket.WideCounter //can't import
  //val L1I_PFC_read = WideCounter(64, io.update.L1I.read)
  //val (L1IPFC_read, full0)     = Counter(io.update.L1I.read,         2^64-1) //wrong why??!!
  val l1ipfc_read           = PerFormanceCounter(io.update.L1I.read,         64)
  val l1ipfc_readmiss       = PerFormanceCounter(io.update.L1I.read_miss,    64)
  val l1dpfc_read           = PerFormanceCounter(io.update.L1D.read,         64)
  val l1dpfc_readmiss       = PerFormanceCounter(io.update.L1D.read_miss,    64)
  val l1dpfc_write          = PerFormanceCounter(io.update.L1D.write,        64)
  val l1dpfc_writemiss      = PerFormanceCounter(io.update.L1D.write_miss,   64)
  val l1dpfc_writeback      = PerFormanceCounter(io.update.L1D.write_back,   64)

  val Reg_l1ipfc_read       = RegEnable(l1ipfc_read,       acquire)
  val Reg_l1ipfc_readmiss   = RegEnable(l1ipfc_readmiss,   acquire)
  val Reg_l1dpfc_read       = RegEnable(l1dpfc_read,       acquire)
  val Reg_l1dpfc_readmiss   = RegEnable(l1dpfc_readmiss,   acquire)
  val Reg_l1dpfc_write      = RegEnable(l1dpfc_write,      acquire)
  val Reg_l1dpfc_writemiss  = RegEnable(l1dpfc_writemiss,  acquire)
  val Reg_l1dpfc_writeback  = RegEnable(l1dpfc_writeback,  acquire)

  val l1ipfc = Reg_l1ipfc_read
  val l1dpfc = Reg_l1dpfc_read
  io.resp.valid  := read
  io.resp.bits.data := Mux(groupisL1I, l1ipfc, l1dpfc)
  switch(io.req.bits.addr) {
  /*  is(UInt(0)) {
      l1ipfc := Reg_l1ipfc_read
      l1dpfc := Reg_l1dpfc_read
   } */
    is(UInt(1)) {
      l1ipfc := Reg_l1ipfc_readmiss
      l1dpfc := Reg_l1dpfc_readmiss
    }
    is(UInt(2)) {
      l1dpfc := Reg_l1dpfc_write
    }
    is(UInt(3)) {
      l1dpfc := Reg_l1dpfc_writemiss
    }
    is(UInt(4)) {
      l1dpfc := Reg_l1dpfc_writeback
    }
 }
}

class SharePFC(implicit p: Parameters) extends PFCModule()(p) {
  val io = new Bundle {
    val req = Valid(new PFCReq()).flip() //req from csr
    val resp = Valid(new PFCResp()) //resp to csr
    val update = new SharePerform()
  }

  require(L2Banks<=8)

  val groupMatch = io.req.valid && io.req.bits.groupID === UInt(2) //4'b0010
  val groupisL2 = groupMatch && io.req.bits.subGroID === UInt(1) //4'b0001
  val groupisTC = groupMatch && io.req.bits.subGroID === UInt(2) //4'b0010
  val read = groupMatch && io.req.bits.cmd === UInt(0)
  val acquire = groupMatch && io.req.bits.cmd === UInt(1)

  //L2
  val l2pfcs     = (0 until L2Banks).map(i => Wire(new L2CachePerformCounter))
  val Reg_l2pfcs = (0 until L2Banks).map(i => Reg(new  L2CachePerformCounter))
    (0 until L2Banks).map(i =>{
    Reg_l2pfcs(i)         := RegEnable(l2pfcs(i), acquire)
    l2pfcs(i).read        := PerFormanceCounter(io.update.L2D(i).read,        64)
    l2pfcs(i).read_miss   := PerFormanceCounter(io.update.L2D(i).read_miss,   64)
    l2pfcs(i).write       := PerFormanceCounter(io.update.L2D(i).write,       64)
    l2pfcs(i).write_back  := PerFormanceCounter(io.update.L2D(i).write_back,  64)
  })

  //TC
  val tcpfc = Wire(new TagCachePerformCounter)
  val Reg_tcpfc = RegEnable(tcpfc, acquire)
  tcpfc.readTT          := PerFormanceCounter(io.update.TAG.readTT,         64)
  tcpfc.readTT_miss     := PerFormanceCounter(io.update.TAG.readTT_miss,    64)
  tcpfc.writeTT         := PerFormanceCounter(io.update.TAG.writeTT,        64)
  tcpfc.writeTT_miss    := PerFormanceCounter(io.update.TAG.writeTT_miss,   64)
  tcpfc.writeTT_back    := PerFormanceCounter(io.update.TAG.writeTT_back,   64)
  tcpfc.readTM0         := PerFormanceCounter(io.update.TAG.readTM0,        64)
  tcpfc.readTM0_miss    := PerFormanceCounter(io.update.TAG.readTM0_miss,   64)
  tcpfc.writeTM0        := PerFormanceCounter(io.update.TAG.writeTM0,       64)
  tcpfc.writeTM0_miss   := PerFormanceCounter(io.update.TAG.writeTM0_miss,  64)
  tcpfc.writeTM0_back   := PerFormanceCounter(io.update.TAG.writeTM0_back,  64)
  tcpfc.readTM1         := PerFormanceCounter(io.update.TAG.readTM1,        64)
  tcpfc.readTM1_miss    := PerFormanceCounter(io.update.TAG.readTM1_miss,   64)
  tcpfc.writeTM1        := PerFormanceCounter(io.update.TAG.writeTM1,       64)
  tcpfc.writeTM1_miss   := PerFormanceCounter(io.update.TAG.writeTM1_miss,  64)
  tcpfc.writeTM1_back   := PerFormanceCounter(io.update.TAG.writeTM1_back,  64)

  val tcpfc_muxed = Reg_tcpfc.readTT
  val l2pfc_muxed = if(L2Banks>0) Reg_l2pfcs(0).read else UInt(0)
  io.resp.valid  := read
  io.resp.bits.data := Mux(groupisL2, l2pfc_muxed, tcpfc_muxed)
  switch(io.req.bits.addr) {
    /*  is(UInt(0)) {
        tcpfc := Reg_tcmtpfc.readTT
        if(L2Banks>0) l2pfc := Reg_l2pfcs(0).read
     } */
    is(UInt(1)) {
      tcpfc_muxed := Reg_tcpfc.readTT_miss
      if (L2Banks > 0) l2pfc_muxed := Reg_l2pfcs(0).read_miss
    }
    is(UInt(2)) {
      tcpfc_muxed := Reg_tcpfc.writeTT
      if (L2Banks > 0) l2pfc_muxed := Reg_l2pfcs(0).write
    }
    is(UInt(3)) {
      tcpfc_muxed := Reg_tcpfc.writeTT_miss
      if (L2Banks > 0) l2pfc_muxed := Reg_l2pfcs(0).write_back
    }
    is(UInt(4)) {
      tcpfc_muxed := Reg_tcpfc.writeTT_back
      if (L2Banks > 1) l2pfc_muxed := Reg_l2pfcs(1).read
    }
    is(UInt(5)) {
      tcpfc_muxed := Reg_tcpfc.readTM0
      if (L2Banks > 1) l2pfc_muxed := Reg_l2pfcs(1).read_miss
    }
    is(UInt(6)) {
      tcpfc_muxed := Reg_tcpfc.readTM0_miss
      if (L2Banks > 1) l2pfc_muxed := Reg_l2pfcs(1).write
    }
    is(UInt(7)) {
      tcpfc_muxed := Reg_tcpfc.writeTM0
      if (L2Banks > 1) l2pfc_muxed := Reg_l2pfcs(1).write_back
    }
    is(UInt(8)) {
      tcpfc_muxed := Reg_tcpfc.writeTM0_miss
      if (L2Banks > 2) l2pfc_muxed := Reg_l2pfcs(2).read
    }
    is(UInt(9)) {
      tcpfc_muxed := Reg_tcpfc.writeTM0_back
      if (L2Banks > 2) l2pfc_muxed := Reg_l2pfcs(2).read_miss
    }
    is(UInt(10)) {
      tcpfc_muxed := Reg_tcpfc.readTM1
      if (L2Banks > 2) l2pfc_muxed := Reg_l2pfcs(2).write
    }
    is(UInt(11)) {
      tcpfc_muxed := Reg_tcpfc.readTM1_miss
      if (L2Banks > 2) l2pfc_muxed := Reg_l2pfcs(2).write_back
    }
    is(UInt(12)) {
      tcpfc_muxed := Reg_tcpfc.writeTM1
      if (L2Banks > 3) l2pfc_muxed := Reg_l2pfcs(3).read
    }
    is(UInt(13)) {
      tcpfc_muxed := Reg_tcpfc.writeTM1_miss
      if (L2Banks > 3) l2pfc_muxed := Reg_l2pfcs(3).read_miss
    }
    is(UInt(14)) {
      tcpfc_muxed := Reg_tcpfc.writeTM1_back
      if (L2Banks > 3) l2pfc_muxed := Reg_l2pfcs(3).write
    }
    is(UInt(15)) {
      if (L2Banks > 3) l2pfc_muxed := Reg_l2pfcs(3).write_back
    }
    is(UInt(16)) {
      if (L2Banks > 4) l2pfc_muxed := Reg_l2pfcs(4).read
    }
    is(UInt(17)) {
      if (L2Banks > 4) l2pfc_muxed := Reg_l2pfcs(4).read_miss
    }
    is(UInt(18)) {
      if (L2Banks > 4) l2pfc_muxed := Reg_l2pfcs(4).write
    }
    is(UInt(19)) {
      if (L2Banks > 4) l2pfc_muxed := Reg_l2pfcs(4).write_back
    }
    is(UInt(20)) {
      if (L2Banks > 5) l2pfc_muxed := Reg_l2pfcs(5).read
    }
    is(UInt(21)) {
      if (L2Banks > 5) l2pfc_muxed := Reg_l2pfcs(5).read_miss
    }
    is(UInt(22)) {
      if (L2Banks > 5) l2pfc_muxed := Reg_l2pfcs(5).write
    }
    is(UInt(23)) {
      if (L2Banks > 5) l2pfc_muxed := Reg_l2pfcs(5).write_back
    }
    is(UInt(24)) {
      if (L2Banks > 6) l2pfc_muxed := Reg_l2pfcs(6).read
    }
    is(UInt(25)) {
      if (L2Banks > 6) l2pfc_muxed := Reg_l2pfcs(6).read_miss
    }
    is(UInt(26)) {
      if (L2Banks > 6) l2pfc_muxed := Reg_l2pfcs(6).write
    }
    is(UInt(27)) {
      if (L2Banks > 6) l2pfc_muxed := Reg_l2pfcs(6).write_back
    }
    is(UInt(28)) {
      if (L2Banks > 7) l2pfc_muxed := Reg_l2pfcs(7).read
    }
    is(UInt(29)) {
      if (L2Banks > 7) l2pfc_muxed := Reg_l2pfcs(7).read_miss
    }
    is(UInt(30)) {
      if (L2Banks > 7) l2pfc_muxed := Reg_l2pfcs(7).write
    }
    is(UInt(31)) {
      if (L2Banks > 7) l2pfc_muxed := Reg_l2pfcs(7).write_back
    }
  }
}

class PFCClientIO(implicit p: Parameters) extends PFCBundle()(p) {
  val req = Decoupled(new PFCReq()).flip()
  val resp = Decoupled(new PFCResp())
}

class PFCManagerIO(implicit p: Parameters) extends PFCBundle()(p) {
  val req = Decoupled(new PFCReq())
  val resp = Decoupled(new PFCResp()).flip()
}

class PFCCrossbar(implicit p: Parameters) extends PFCModule()(p) {
  val io  = new Bundle {
    val clients  = Vec(Clients, new PFCClientIO())
    val managers = Vec(Managers, new PFCManagerIO())
  }
  val reqNet  = Module(new BasicCrossbar(NetPorts, new PFCReq, count=1, Some((req: PhysicalNetworkIO[PFCReq]) => req.payload.hasMultibeatData())))
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
  (0 until Managers).map(i => {
    //reqNet.out to pfc.req
    io.managers(i).req.valid          := reqNet.io.out(i).valid
    reqNet.io.out(i).ready             := io.managers(i).req.ready
    io.managers(i).req.bits           := reqNet.io.out(i).bits.payload
    //pfc.resp to respNet.in
    respNet.io.in(i).valid            := io.managers(i).resp.valid
    io.managers(i).resp.ready         := respNet.io.in(i).ready
    respNet.io.in(i).bits.payload     := io.managers(i).resp.bits
    respNet.io.in(i).bits.header.src  := io.managers(i).resp.bits.src
    respNet.io.in(i).bits.header.dst  := io.managers(i).resp.bits.dst
  })

}
