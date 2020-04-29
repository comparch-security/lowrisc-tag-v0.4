// See LICENSE for license details.

package uncore

import Chisel._
import cde.{Parameters, Field}

case object PFCL2N extends Field[Int]

trait HasPFCParameters extends HasTagParameters {
  implicit val p: Parameters
  val L2DBanks = p(PFCL2N)
}

abstract class PFCModule(implicit val p: Parameters) extends Module with HasPFCParameters
abstract class PFCBundle(implicit val p: Parameters) extends Bundle with HasPFCParameters

object PerFormanceCounter {
  def apply(cond: Bool, n: Int): UInt = {
    val c = new Counter(n)
    var wrap: Bool = null
    when (cond) { wrap = c.inc() }
    c.value
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

class L2DCachePerform extends Bundle {
  val read = Bool(INPUT) //inner.acquire
  val read_miss = Bool(INPUT) //outer.acquire
  val write =Bool(INPUT) //inner.release
  val write_miss =Bool(INPUT) //inner.release
  val write_back = Bool(INPUT) //outer.release
}

class L2DCachePerformCounter extends Bundle {
   val read       = UInt(width=64)
   val read_miss  = UInt(width=64)
   val write      = UInt(width=64)
   val write_miss = UInt(width=64)
   val write_back = UInt(width=64)
}

class TCTAGTrackerPerform extends Bundle {
  val MR  = Bool(INPUT)        //read meta
  val MW  = Bool(INPUT)        //update the meta
  val DR  = Bool(INPUT)        //read tag from data array
  val DW  = Bool(INPUT)        //write tag to data array //equals DWR+DWB
  val WB  = Bool(INPUT)        //write dirty line back to memory
  val F   = Bool(INPUT)        //fetch the target cache line (from memory) // equals DWB
}

class TCTAGTrackerPerformCounter extends Bundle {
  val MR  = UInt(width=64)        //read meta
  val MW  = UInt(width=64)        //update the meta
  val DR  = UInt(width=64)        //read tag from data array
  val DW  = UInt(width=64)        //write tag to data array //equals DWR+DWB
  val WB  = UInt(width=64)        //write dirty line back to memory
  val F   = UInt(width=64)        //fetch the target cache line (from memory) // equals DWB
}

class TCMEMTrackerPerform extends Bundle {
  val readTT           = Bool(INPUT)
  val readTT_miss      = Bool(INPUT)
  val writeTT          = Bool(INPUT)
  val writeTT_miss     = Bool(INPUT)
  val readTM0          = Bool(INPUT)
  val readTM0_miss     = Bool(INPUT)
  val writeTM0         = Bool(INPUT)
  val writeTM0_miss    = Bool(INPUT)
  val readTM1          = Bool(INPUT)
  val readTM1_miss     = Bool(INPUT)
  val writeTM1         = Bool(INPUT)
  val writeTM1_miss    = Bool(INPUT)
}

class TCMEMTrackerPerformCounter extends Bundle {
  val  readTT          = UInt(width=64)
  val  readTT_miss     = UInt(width=64)
  val  writeTT         = UInt(width=64)
  val  writeTT_miss    = UInt(width=64)
  val  readTM0         = UInt(width=64)
  val  readTM0_miss    = UInt(width=64)
  val  writeTM0        = UInt(width=64)
  val  writeTM0_miss   = UInt(width=64)
  val  readTM1         = UInt(width=64)
  val  readTM1_miss    = UInt(width=64)
  val  writeTM1        = UInt(width=64)
  val  writeTM1_miss   = UInt(width=64)
}

class TAGCachePerform extends Bundle {
  val tcttp = new TCTAGTrackerPerform()//
  val tcmtp = new TCMEMTrackerPerform()
}

class PrivatePerform extends Bundle {
  val L1I = new L1ICachePerform()
  val L1D = new L1DCachePerform()
}

//class SharePerform(implicit val p: Parameters) extends PFCBundle { //WRONG
//  val L2D = Vec(L2DBanks, new L2DCachePerform)
class SharePerform(implicit val p: Parameters) extends Bundle {
  val L2D = Vec(p(PFCL2N), new L2DCachePerform)
  val TAG = new TAGCachePerform()
}

class PrivatePFC extends Module {
  val io = new Bundle {
    //val req = //req from csr
    //val resp = //resp to csr
    val update = new PrivatePerform()
  }

  val acquire = Bool()

  //import rocket.WideCounter //can't import
  //val L1I_PFC_read = WideCounter(64, io.update.L1I.read)
  //val (L1IPFC_read, full0)     = Counter(io.update.L1I.read,         2^64-1) //wrong why??!!
  val l1ipfc_read           = PerFormanceCounter(io.update.L1I.read,         2^64-1)
  val l1ipfc_readmiss       = PerFormanceCounter(io.update.L1I.read_miss,    2^64-1)
  val l1dpfc_read           = PerFormanceCounter(io.update.L1D.read,         2^64-1)
  val l1dpfc_readmiss       = PerFormanceCounter(io.update.L1D.read_miss,    2^64-1)
  val l1dpfc_write          = PerFormanceCounter(io.update.L1D.write,        2^64-1)
  val l1dpfc_writemiss      = PerFormanceCounter(io.update.L1D.write_miss,   2^64-1)
  val l1dpfc_writeback      = PerFormanceCounter(io.update.L1D.write_back,   2^64-1)

  val Reg_l1ipfc_read       = RegEnable(l1ipfc_read,       acquire)
  val Reg_l1ipfc_readmiss   = RegEnable(l1ipfc_readmiss,   acquire)
  val Reg_L1IPFC_read       = RegEnable(l1dpfc_read,       acquire)
  val Reg_L1DPFC_readmiss   = RegEnable(l1dpfc_readmiss,   acquire)
  val Reg_L1DPFC_write      = RegEnable(l1dpfc_write,      acquire)
  val Reg_L1DPFC_writemiss  = RegEnable(l1dpfc_writemiss,  acquire)
  val Reg_L1DPFC_writeback  = RegEnable(l1dpfc_writeback,  acquire)
}

class SharePFC(implicit val p: Parameters) extends Module {
  val io = new Bundle {
    //val req = //req from csr
    //val resp = //resp to csr
    val update = new SharePerform()
  }

  val L2DBanks = p(PFCL2N)
  val acquire = Bool()

  //L2D
  val l2dpfcs     = (0 until L2DBanks).map(i => Wire(new L2DCachePerformCounter))
  val Reg_l2dpfcs = (0 until L2DBanks).map(i => Reg(new  L2DCachePerformCounter))
    (0 until L2DBanks).map(i =>{
    Reg_l2dpfcs(i)         := RegEnable(l2dpfcs(i), acquire)
    l2dpfcs(i).read        := PerFormanceCounter(io.update.L2D(i).read.toBool(),        2^64-1)
    l2dpfcs(i).read_miss   := PerFormanceCounter(io.update.L2D(i).read_miss.toBool(),   2^64-1)
    l2dpfcs(i).write       := PerFormanceCounter(io.update.L2D(i).write.toBool(),       2^64-1)
    l2dpfcs(i).write_miss  := PerFormanceCounter(io.update.L2D(i).write.toBool(),       2^64-1)
    l2dpfcs(i).write_back  := PerFormanceCounter(io.update.L2D(i).write_back.toBool(),  2^64-1)
  })

  //TAG
  val tcttpfc = Wire(new TCTAGTrackerPerformCounter)
  val tcmtpfc = Wire(new TCMEMTrackerPerformCounter)
  val Reg_tcttpfc = RegEnable(tcttpfc, acquire)
  val Reg_tcmtpfc = RegEnable(tcmtpfc, acquire)
  tcttpfc.MR          := PerFormanceCounter(io.update.TAG.tcttp.MR.toBool(),          2^64-1)
  tcttpfc.DR          := PerFormanceCounter(io.update.TAG.tcttp.DR.toBool(),          2^64-1)
  tcttpfc.DW          := PerFormanceCounter(io.update.TAG.tcttp.DW.toBool(),          2^64-1)
  tcttpfc.WB          := PerFormanceCounter(io.update.TAG.tcttp.WB.toBool(),          2^64-1)
  tcttpfc.F           := PerFormanceCounter(io.update.TAG.tcttp.F.toBool(),           2^64-1)
  tcmtpfc.readTT          := PerFormanceCounter(io.update.TAG.tcmtp.readTT.toBool(),         2^64-1)
  tcmtpfc.readTT_miss     := PerFormanceCounter(io.update.TAG.tcmtp.readTT_miss.toBool(),    2^64-1)
  tcmtpfc.writeTT         := PerFormanceCounter(io.update.TAG.tcmtp.writeTT.toBool(),        2^64-1)
  tcmtpfc.writeTT_miss    := PerFormanceCounter(io.update.TAG.tcmtp.writeTT_miss.toBool(),   2^64-1)
  tcmtpfc.readTM0         := PerFormanceCounter(io.update.TAG.tcmtp.readTM0.toBool(),        2^64-1)
  tcmtpfc.readTM0_miss    := PerFormanceCounter(io.update.TAG.tcmtp.readTM0_miss.toBool(),   2^64-1)
  tcmtpfc.writeTM0        := PerFormanceCounter(io.update.TAG.tcmtp.writeTM0.toBool(),       2^64-1)
  tcmtpfc.writeTM0_miss   := PerFormanceCounter(io.update.TAG.tcmtp.writeTM0_miss.toBool(),  2^64-1)
  tcmtpfc.readTM1         := PerFormanceCounter(io.update.TAG.tcmtp.readTM1.toBool(),        2^64-1)
  tcmtpfc.readTM1_miss    := PerFormanceCounter(io.update.TAG.tcmtp.readTM1_miss.toBool(),   2^64-1)
  tcmtpfc.writeTM1        := PerFormanceCounter(io.update.TAG.tcmtp.writeTM1.toBool(),       2^64-1)
  tcmtpfc.writeTM1_miss   := PerFormanceCounter(io.update.TAG.tcmtp.writeTM1_miss.toBool(),  2^64-1)
}
