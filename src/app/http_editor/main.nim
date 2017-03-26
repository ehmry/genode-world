import asyncfile, asynchttpserver, asyncdispatch, strutils, os
from htmlgen import nil

import posix

const
  FormInner = htmlgen.textarea(
      cols="80", rows="40",
      spellcheck="false",
      name="text",
      "$#"
    ) &
    htmlgen.input(type="reset", value="Reset") &
    htmlgen.input(type="submit", value="Submit")
  FormFormat = """<form action="$#" method="post" enctype="text/plain">""" & FormInner & "</form>"
  GetFormat = htmlgen.html(htmlgen.body(FormFormat))
  # All that because 'method' is a Nim keyword

var server = newAsyncHttpServer()

proc get(req: Request) {.async.} =
  var file = openAsync(req.url.path, fmReadWrite)
  let text = await file.readAll()
  file.close()
  await req.respond(Http200, htmlgen.html(GetFormat % [req.url.path, text]))

proc post(req: Request) {.async.} =
  const PostPrefix = "text="
  let body = (if req.body.startsWith(PostPrefix):
    req.body[PostPrefix.len..req.body.high]
  else:
    req.body)
  var file = openAsync(req.url.path, fmWrite)
  file.setLen(body.len)
  await file.write(body)
  await get(req)
  # reload the file and the editor for the client

proc methodHandler(req: Request) {.async.} =
  let fut = case req.reqMethod:
  of HttpGet:
    get(req)
  of HttpPost:
    post(req)
  else:
    req.respond(Http501, "only GET and POST methods are supported")

  yield fut
  if fut.failed:
    await req.respond(Http500, getCurrentExceptionMsg())

waitFor server.serve(Port(80), methodHandler)
