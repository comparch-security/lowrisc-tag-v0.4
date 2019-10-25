organization := "edu.berkeley.cs"

version := "1.0"

name := "cde"

scalaVersion := "2.11.6"

// Provide a managed dependency on chisel if -DchiselVersion="" issupplied on the command line.
libraryDependencies ++= (Seq("chisel").map {
  dep: String => sys.props.get(dep + "Version") map { "edu.berkeley.cs" %% dep % _ }}).flatten
