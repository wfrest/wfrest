export_header_list = {
    "src/base/Copyable.h",
    "src/base/ErrorCode.h",
    "src/base/json_fwd.hpp",
    "src/base/json.hpp",
    "src/base/Macro.h",
    "src/base/Noncopyable.h",
    "src/base/StringPiece.h",
    "src/base/Timestamp.h",
    "src/base/base64.h",
    "src/base/Compress.h",
    "src/base/SysInfo.h",

    "src/core/HttpContent.h",
    "src/core/HttpCookie.h",
    "src/core/HttpDef.h",
    "src/core/HttpFile.h",
    "src/core/HttpMsg.h",
    "src/core/HttpServer.h", 
    "src/core/HttpServerTask.h",
    "src/core/MultiPartParser.h",
    "src/core/BluePrint.h",
    "src/core/BluePrint.inl",
    "src/core/Router.h",
    "src/core/RouteTable.h",
    "src/core/VerbHandler.h",
	"src/core/AopUtil.h",
    "src/core/Aspect.h",

    "src/util/FileUtil.h",
    "src/util/MysqlUtil.h",
    "src/util/PathUtil.h",
    "src/util/StrUtil.h",
    "src/util/UriUtil.h",
    "src/util/CodeUtil.h",
}

function main(target)
  if (not os.isdir("$(projectdir)/_include/")) then
    os.mkdir("$(projectdir)/_include/wfrest/")
    for i = 1, #export_header_list do
      os.cp("$(projectdir)/" .. export_header_list[i], "$(projectdir)/_include/wfrest/")
    end
  end
end
