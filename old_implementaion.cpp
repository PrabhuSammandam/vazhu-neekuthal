#if 0
void GdbMgr::readTokens() {
   auto *myprocess = dynamic_cast<MyProcess *>(_gdbProcess);

   wxString m_inputBuffer;

   myprocess->ReadAll(m_inputBuffer);

   if (m_inputBuffer.Contains("*stopped")) {
      int a = 0;
      a++;
   }

   auto bufferSize = m_inputBuffer.length();

   // Any characters received?
   while (!m_inputBuffer.empty()) {
      std::cout << m_inputBuffer << std::endl;

      // Newline received?
      auto subLen = m_inputBuffer.Index('\n');
      if (subLen != -1) {
         wxString row  = wxString(m_inputBuffer.Left(subLen));
         m_inputBuffer = m_inputBuffer.Mid(subLen + 1);
         bufferSize    = m_inputBuffer.length();

         if (!row.IsEmpty()) {
            std::vector<Token *> list;
            char                 firstChar = row[0];

            // clang-format off
            if (firstChar == '(' ||
                firstChar == '^' ||
                firstChar == '*' ||
                firstChar == '+' ||
                firstChar == '~' ||
                firstChar == '@' ||
                firstChar == '&' ||
                firstChar == '=')
            {
               list = tokenize(row);
               m_list.insert(m_list.end(), list.begin(), list.end());
            } else {
               onTargetStreamOutput(row);
            }
            // clang-format on
         }
      }

      // Half a line received?
      if (m_inputBuffer.IsEmpty() == false) {
         int timeout = 20;
         // Wait for the complete line to be received
         while (m_inputBuffer.Index('\n') == -1) {
            wxMilliSleep(100);
            wxString tempOutput;
            myprocess->ReadAll(tempOutput);

            if (!tempOutput.IsEmpty()) {
               m_inputBuffer += tempOutput;
            }
            timeout--;
         }
      }
   }
}
class MyProcess : public wxProcess {
 public:
   explicit MyProcess(GdbMgr *parent) : wxProcess{wxPROCESS_REDIRECT}, _parent{parent} {}
   void OnTerminate(int pid, int status) override { _parent->onGdbProcessTerminate(pid, status); }

   void write(const wxString &text) {
      wxString s(text);
      s += '\n';
      auto *out = GetOutputStream();
      out->Write(s.c_str(), s.length());
   }

   auto HasInput() -> bool {
      if (IsInputAvailable()) {
         return true;
      }
      if (IsErrorAvailable()) {
         return true;
      }
      return false;
   }

   auto HasInput(wxString &input) -> bool {
      bool hasInput = false;
      bool cont1(true);
      bool cont2(true);
      while (cont1 || cont2) {
         cont1 = false;
         cont2 = false;
         while (IsInputAvailable()) {
            wxTextInputStream tis(*GetInputStream());
            // this assumes that the output is always line buffered
            wxChar ch = tis.GetChar();
            input << ch;
            hasInput = true;
            if (ch == wxT('\n')) {
               cont1 = false;
               break;
            }
            cont1 = true;
         }

         while (IsErrorAvailable()) {
            wxTextInputStream tis(*GetErrorStream());
            // this assumes that the output is always line buffered
            wxChar ch = tis.GetChar();
            input << ch;
            hasInput = true;
            if (ch == wxT('\n')) {
               cont2 = false;
               break;
            }
            cont2 = true;
         }
      }
      return hasInput;
   }

   auto ReadAll(wxString &input) -> bool {
      bool hasInput = false;
      bool cont1(true);
      bool cont2(true);

      wxTextInputStream tis(*GetInputStream());
      wxTextInputStream tie(*GetErrorStream());
      while (cont1 || cont2) {
         cont1 = false;
         cont2 = false;
         while (IsInputAvailable()) {
            // this assumes that the output is always line buffered
            wxChar ch = tis.GetChar();
            input << ch;
            hasInput = true;
            cont1    = true;
         }

         while (IsErrorAvailable()) {
            // this assumes that the output is always line buffered
            wxChar ch = tie.GetChar();
            input << ch;
            hasInput = true;
            cont2    = true;
         }
      }
      return hasInput;
   }

   GdbMgr *_parent = nullptr;
};
#endif

#if 0
auto GdbMgr::parseStreamRecord() -> GdbResponse * {
   GdbResponse *resp = nullptr;
   Token *      tok  = nullptr;

   if (checkToken(Token::KEY_TILDE) != nullptr) {
      resp = new GdbResponse{GdbResponse::CONSOLE_STREAM_OUTPUT};
      tok  = eatToken(Token::C_STRING);
      resp->setString(tok->getString());

   } else if (checkToken(Token::KEY_SNABEL) != nullptr) {
      resp = new GdbResponse{GdbResponse::TARGET_STREAM_OUTPUT};
      tok  = eatToken(Token::C_STRING);
      resp->setString(tok->getString());

   } else if (checkToken(Token::KEY_AND) != nullptr) {
      resp = new GdbResponse{GdbResponse::LOG_STREAM_OUTPUT};
      tok  = eatToken(Token::C_STRING);
      resp->setString(tok->getString());
   }

   return resp;
}

auto GdbMgr::parseExecAsyncOutput() -> GdbResponse * {
   if (m_list.size() >= 2 && m_list.at(0)->getType() == Token::VAR && m_list.at(0)->getType() != Token::KEY_STAR) {
      return nullptr;
   }

   checkToken(Token::VAR);

   if (checkToken(Token::KEY_STAR) == nullptr) {
      return nullptr;
   }

   auto *resp = new GdbResponse{GdbResponse::EXEC_ASYNC_OUTPUT};

   parseAsyncOutput(resp, &resp->reason);

   return resp;
}

auto GdbMgr::parseNotifyAsyncOutput() -> GdbResponse * {
   if (m_list.size() >= 2 && m_list.at(0)->getType() == Token::VAR && m_list.at(0)->getType() != Token::KEY_EQUAL) {
      return nullptr;
   }
   checkToken(Token::VAR);

   if (checkToken(Token::KEY_EQUAL) == nullptr) {
      return nullptr;
   }

   auto *resp = new GdbResponse{GdbResponse::NOTIFY_ASYNC_OUTPUT};

   parseAsyncOutput(resp, &resp->reason);
   return resp;
}

auto GdbMgr::parseStatusAsyncOutput() -> GdbResponse * {
   if (m_list.size() >= 2 && m_list.at(0)->getType() == Token::VAR && m_list.at(0)->getType() != Token::KEY_PLUS) {
      return nullptr;
   }
   checkToken(Token::VAR);

   if (checkToken(Token::KEY_PLUS) == nullptr) {
      return nullptr;
   }

   auto *resp = new GdbResponse{GdbResponse::STATUS_ASYNC_OUTPUT};

   parseAsyncOutput(resp, &resp->reason);
   return resp;
}
#endif
#if 0
auto GdbMgr::parseOutOfBandRecord() -> GdbResponse * {
   GdbResponse *resp = nullptr;

   if (isTokenPending() && resp == nullptr) {
      resp = parseAsyncRecord();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseStreamRecord();
   }

   return resp;
}

auto GdbMgr::parseAsyncRecord() -> GdbResponse * {
   GdbResponse *resp = nullptr;

   if (isTokenPending() && resp == nullptr) {
      resp = parseExecAsyncOutput();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseStatusAsyncOutput();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseNotifyAsyncOutput();
   }

   return resp;
}
#if 0
auto GdbMgr::parseOutOfBandRecord() -> GdbResponse * {
   GdbResponse *resp = nullptr;

   if (isTokenPending() && resp == nullptr) {
      resp = parseAsyncRecord();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseStreamRecord();
   }

   return resp;
}

auto GdbMgr::parseAsyncRecord() -> GdbResponse * {
   GdbResponse *resp = nullptr;

   if (isTokenPending() && resp == nullptr) {
      resp = parseExecAsyncOutput();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseStatusAsyncOutput();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseNotifyAsyncOutput();
   }

   return resp;
}
#endif

#if 0
   if (!m_gdbOutputArr.IsEmpty()) {
      for (auto gdbOutput : m_gdbOutputArr) {
         char firstChar = gdbOutput[0];

         if (firstChar == '*') { // exec-async-output
            if (gdbOutput.StartsWith("*running")) {
            } else if (gdbOutput.StartsWith("*stopped")) {
               gdbOutput   = gdbOutput.Mid(wxString("*stopped,reason=").Len());
               auto reason = gdbOutput.SubString(0, gdbOutput.Find(",") - 1);
               gdbOutput   = gdbOutput.Mid(reason.Len() + 1); // + 1 for ,

               wxGDB_STRIP_QUOATES(reason);

               if (reason == "breakpoint-hit") {
                  while (!gdbOutput.IsEmpty()) {
                     auto key  = gdbOutput.SubString(0, gdbOutput.Find("=") - 1);
                     gdbOutput = gdbOutput.Mid(key.Len() + 1); // + 1 for =

                     if (key == "disp") {
                        auto value = gdbOutput.SubString(0, gdbOutput.Find(",") - 1);
                        gdbOutput  = gdbOutput.Mid(value.Len() + 1); // + 1 for ,
                        wxGDB_STRIP_QUOATES(value);
                        continue;
                     }
                     if (key == "bkptno") {
                        auto value = gdbOutput.SubString(0, gdbOutput.Find(",") - 1);
                        gdbOutput  = gdbOutput.Mid(value.Len() + 1); // + 1 for ,
                        wxGDB_STRIP_QUOATES(value);
                        continue;
                     }
                     if (key == "frame") {
                        auto value = gdbOutput.SubString(0, gdbOutput.Find(",") - 1);
                        gdbOutput  = gdbOutput.Mid(value.Len() + 1); // + 1 for ,
                        wxGDB_STRIP_QUOATES(value);
                        continue;
                     }
                  }
               }

               wxArrayString result = wxStringTokenize(gdbOutput, ",", wxTOKEN_STRTOK);
               for (auto i = 1; i < result.size(); i++) {
                  auto value_pair = wxStringTokenize(result[i], "=", wxTOKEN_STRTOK);

                  if (value_pair[0] == "reason") {
                     continue;
                  }
                  if (value_pair[0] == "thread-id") {
                     continue;
                  }
                  if (value_pair[0] == "stopped-threads") {
                     continue;
                  }
                  if (value_pair[0] == "core") {
                     continue;
                  }
               }
            }
         } else if (firstChar == '+') { // status-async-output
         } else if (firstChar == '=') { // notify-async-output
         } else if (firstChar == '~') { // console-stream-output
         } else if (firstChar == '@') { // target-stream-output
         } else if (firstChar == '&') { // log-stream-output
         } else if (firstChar == '^') { // result-record
            if (gdbOutput.StartsWith("^done")) {
            } else if (gdbOutput.StartsWith("^running")) {
            } else if (gdbOutput.StartsWith("^connected")) {
            } else if (gdbOutput.StartsWith("^error")) {
            } else if (gdbOutput.StartsWith("^exit")) {
            }
         }
      }
   }
#endif

#if 0
class GdbResponseListener {
 public:
   GdbResponseListener(const GdbResponseListener &other) = delete;
   GdbResponseListener(GdbResponseListener &&other)      = delete;
   auto operator=(const GdbResponseListener &other) -> GdbResponseListener & = delete;
   auto operator=(GdbResponseListener &&other) -> GdbResponseListener & = delete;
   virtual ~GdbResponseListener()                                       = default;

   virtual void onStatusAsyncOut(Tree &tree, GDB_ASYNC_RECORD_TYPE ac) = 0;
   virtual void onNotifyAsyncOut(Tree &tree, GDB_ASYNC_RECORD_TYPE ac) = 0;
   virtual void onExecAsyncOut(Tree &tree, GDB_ASYNC_RECORD_TYPE ac)   = 0;
   virtual void onResult(Tree &tree)                                   = 0;
   virtual void onConsoleStreamOutput(wxString str)                    = 0;
   virtual void onTargetStreamOutput(wxString str)                     = 0;
   virtual void onLogStreamOutput(wxString str)                        = 0;
};
#endif
