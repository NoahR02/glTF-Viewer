package gltf

import "core:encoding/json"
import "core:encoding/base64"

import "core:strings"
import "core:path/filepath"
import "core:fmt"
import "core:slice"
import "core:os"
import "core:mem"
import "core:strconv"
base_64_header :: "data:application/octet-stream;base64,"

load :: proc(path: string) -> (gltf_data: json.Value) {

  if filepath.ext(path) != ".gltf" {
    fmt.eprintln("Currently only .gltf files are supported.")
    return nil
  }
  gltf_data_str_bytes, loaded_file := os.read_entire_file(path)
  if !loaded_file {
    fmt.eprintln("FAILED TO READ:", path)
    return nil
  }  

  json_value, loaded_json := json.parse(data = gltf_data_str_bytes, spec = json.DEFAULT_SPECIFICATION, parse_integers = true)

  if loaded_json != .None {
    fmt.eprintln(loaded_json)
    return nil
  }

  gltf_data = nil

  // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#asset
  // 3.2. Asset
  // Each glTF asset MUST have an asset property.
  // The asset object MUST contain a version property that specifies the target glTF version of the asset.
  asset, valid_asset := json_value.(json.Object)["asset"]
  if !valid_asset {
    fmt.eprintln("FIXME: Return missing asset error")
    return
  }
  version, valid_version := asset.(json.Object)["version"]
  if !valid_version {
    fmt.eprintln("FIXME: Return missing version error")
    return
  }

  gltf_data = json_value

  return
}

str_starts_with :: proc(str: string, sub_str: string) -> bool {
  if len(str) < len(sub_str) do return false
  return str[:len(sub_str)] == sub_str[:]
}

decode_buffer :: proc(buffer: json.Value) -> []byte {
  uri := buffer.(json.Object)["uri"].(json.String)
  byte_length := buffer.(json.Object)["byteLength"].(json.Integer)

  if(str_starts_with(uri, base_64_header)) {
    return base64.decode(uri[len(base_64_header):])
  } else {
    panic("Unknown buffer uri")
  }

  return nil
  
}