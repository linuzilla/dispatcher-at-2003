object MainForm: TMainForm
  Left = 192
  Top = 141
  BorderStyle = bsDialog
  Caption = '分發程式 - << Jiann-Ching Liu >>'
  ClientHeight = 444
  ClientWidth = 688
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Scaled = False
  OnCreate = FormCreate
  PixelsPerInch = 120
  TextHeight = 16
  object Label2: TLabel
    Left = 8
    Top = 32
    Width = 100
    Height = 25
    Caption = '學生志願檔'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -20
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
  end
  object Label6: TLabel
    Left = 8
    Top = 56
    Width = 100
    Height = 25
    Caption = '學生成績檔'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -20
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 688
    Height = 193
    Align = alTop
    BorderWidth = 2
    BorderStyle = bsSingle
    TabOrder = 0
    object Label1: TLabel
      Left = 8
      Top = 8
      Width = 100
      Height = 25
      Caption = '學生志願檔'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -20
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Label3: TLabel
      Left = 8
      Top = 32
      Width = 100
      Height = 25
      Caption = '學生資料檔'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -20
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Label4: TLabel
      Left = 8
      Top = 56
      Width = 100
      Height = 25
      Caption = '學生成績檔'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -20
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Label5: TLabel
      Left = 8
      Top = 80
      Width = 100
      Height = 25
      Caption = '校系限制檔'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -20
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Label7: TLabel
      Left = 8
      Top = 160
      Width = 100
      Height = 25
      Caption = '結果輸出檔'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -20
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Label8: TLabel
      Left = 8
      Top = 104
      Width = 100
      Height = 25
      Caption = '各種標準檔'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -20
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Label9: TLabel
      Left = 8
      Top = 128
      Width = 100
      Height = 25
      Caption = '分發禁止檔'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -20
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object BitBtnRun: TBitBtn
      Left = 408
      Top = 104
      Width = 137
      Height = 81
      Caption = 'Run'
      TabOrder = 0
      OnClick = BitBtnRunClick
      Kind = bkOK
    end
    object BitBtnClose: TBitBtn
      Left = 544
      Top = 144
      Width = 129
      Height = 41
      TabOrder = 1
      OnClick = BitBtnCloseClick
      Kind = bkClose
    end
    object BitBtnAbout: TBitBtn
      Left = 544
      Top = 104
      Width = 129
      Height = 41
      Caption = '&About'
      TabOrder = 2
      OnClick = BitBtnAboutClick
      Glyph.Data = {
        DE010000424DDE01000000000000760000002800000024000000120000000100
        0400000000006801000000000000000000001000000000000000000000000000
        80000080000000808000800000008000800080800000C0C0C000808080000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333333336633
        3333333333333FF3333333330000333333364463333333333333388F33333333
        00003333333E66433333333333338F38F3333333000033333333E66333333333
        33338FF8F3333333000033333333333333333333333338833333333300003333
        3333446333333333333333FF3333333300003333333666433333333333333888
        F333333300003333333E66433333333333338F38F333333300003333333E6664
        3333333333338F38F3333333000033333333E6664333333333338F338F333333
        0000333333333E6664333333333338F338F3333300003333344333E666433333
        333F338F338F3333000033336664333E664333333388F338F338F33300003333
        E66644466643333338F38FFF8338F333000033333E6666666663333338F33888
        3338F3330000333333EE666666333333338FF33333383333000033333333EEEE
        E333333333388FFFFF8333330000333333333333333333333333388888333333
        0000}
      NumGlyphs = 2
    end
    object RadioGroup1: TRadioGroup
      Left = 408
      Top = 8
      Width = 265
      Height = 89
      Caption = '輸出格式'
      TabOrder = 3
    end
    object RadioButton1: TRadioButton
      Left = 440
      Top = 44
      Width = 73
      Height = 25
      Caption = '長表'
      Checked = True
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -20
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 4
      TabStop = True
      OnClick = RadioButton1Click
    end
    object RadioButton2: TRadioButton
      Left = 560
      Top = 40
      Width = 73
      Height = 33
      Caption = '榜單'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -20
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 5
      OnClick = RadioButton2Click
    end
  end
  object Memo1: TMemo
    Left = 0
    Top = 193
    Width = 688
    Height = 251
    TabStop = False
    Align = alClient
    Font.Charset = ANSI_CHARSET
    Font.Color = clWindowText
    Font.Height = -15
    Font.Name = 'Courier New'
    Font.Style = []
    ParentFont = False
    ReadOnly = True
    ScrollBars = ssBoth
    TabOrder = 1
  end
  object EditStdinfo: TEdit
    Left = 120
    Top = 32
    Width = 201
    Height = 24
    TabOrder = 2
    Text = 'stdinfo.txt'
  end
  object BitBtnStdinfo: TBitBtn
    Left = 328
    Top = 32
    Width = 75
    Height = 25
    Caption = '瀏覽'
    TabOrder = 3
    OnClick = BitBtnStdinfoClick
    Glyph.Data = {
      76010000424D7601000000000000760000002800000020000000100000000100
      04000000000000010000130B0000130B00001000000000000000000000000000
      800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
      FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF0033333333B333
      333B33FF33337F3333F73BB3777BB7777BB3377FFFF77FFFF77333B000000000
      0B3333777777777777333330FFFFFFFF07333337F33333337F333330FFFFFFFF
      07333337F3FF3FFF7F333330F00F000F07333337F77377737F333330FFFFFFFF
      07333FF7F3FFFF3F7FFFBBB0F0000F0F0BB37777F7777373777F3BB0FFFFFFFF
      0BBB3777F3FF3FFF77773330F00F000003333337F773777773333330FFFF0FF0
      33333337F3FF7F37F3333330F08F0F0B33333337F7737F77FF333330FFFF003B
      B3333337FFFF77377FF333B000000333BB33337777777F3377FF3BB3333BB333
      3BB33773333773333773B333333B3333333B7333333733333337}
    NumGlyphs = 2
  end
  object EditScore: TEdit
    Left = 120
    Top = 56
    Width = 201
    Height = 24
    TabOrder = 4
    Text = 'score.txt'
  end
  object BitBtnScore: TBitBtn
    Left = 328
    Top = 56
    Width = 75
    Height = 25
    Caption = '瀏覽'
    TabOrder = 5
    OnClick = BitBtnScoreClick
    Glyph.Data = {
      76010000424D7601000000000000760000002800000020000000100000000100
      04000000000000010000130B0000130B00001000000000000000000000000000
      800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
      FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF0033333333B333
      333B33FF33337F3333F73BB3777BB7777BB3377FFFF77FFFF77333B000000000
      0B3333777777777777333330FFFFFFFF07333337F33333337F333330FFFFFFFF
      07333337F3FF3FFF7F333330F00F000F07333337F77377737F333330FFFFFFFF
      07333FF7F3FFFF3F7FFFBBB0F0000F0F0BB37777F7777373777F3BB0FFFFFFFF
      0BBB3777F3FF3FFF77773330F00F000003333337F773777773333330FFFF0FF0
      33333337F3FF7F37F3333330F08F0F0B33333337F7737F77FF333330FFFF003B
      B3333337FFFF77377FF333B000000333BB33337777777F3377FF3BB3333BB333
      3BB33773333773333773B333333B3333333B7333333733333337}
    NumGlyphs = 2
  end
  object EditDeplim: TEdit
    Left = 120
    Top = 80
    Width = 201
    Height = 24
    TabOrder = 6
    Text = 'deplim.txt'
  end
  object BitBtnDeplim: TBitBtn
    Left = 328
    Top = 80
    Width = 75
    Height = 25
    Caption = '瀏覽'
    TabOrder = 7
    OnClick = BitBtnDeplimClick
    Glyph.Data = {
      76010000424D7601000000000000760000002800000020000000100000000100
      04000000000000010000130B0000130B00001000000000000000000000000000
      800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
      FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF0033333333B333
      333B33FF33337F3333F73BB3777BB7777BB3377FFFF77FFFF77333B000000000
      0B3333777777777777333330FFFFFFFF07333337F33333337F333330FFFFFFFF
      07333337F3FF3FFF7F333330F00F000F07333337F77377737F333330FFFFFFFF
      07333FF7F3FFFF3F7FFFBBB0F0000F0F0BB37777F7777373777F3BB0FFFFFFFF
      0BBB3777F3FF3FFF77773330F00F000003333337F773777773333330FFFF0FF0
      33333337F3FF7F37F3333330F08F0F0B33333337F7737F77FF333330FFFF003B
      B3333337FFFF77377FF333B000000333BB33337777777F3377FF3BB3333BB333
      3BB33773333773333773B333333B3333333B7333333733333337}
    NumGlyphs = 2
  end
  object EditWish: TEdit
    Left = 120
    Top = 8
    Width = 201
    Height = 24
    TabOrder = 8
    Text = 'wish.txt'
  end
  object BitBtnWish: TBitBtn
    Left = 328
    Top = 8
    Width = 75
    Height = 25
    Caption = '瀏覽'
    TabOrder = 9
    OnClick = BitBtnWishClick
    Glyph.Data = {
      76010000424D7601000000000000760000002800000020000000100000000100
      04000000000000010000130B0000130B00001000000000000000000000000000
      800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
      FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF0033333333B333
      333B33FF33337F3333F73BB3777BB7777BB3377FFFF77FFFF77333B000000000
      0B3333777777777777333330FFFFFFFF07333337F33333337F333330FFFFFFFF
      07333337F3FF3FFF7F333330F00F000F07333337F77377737F333330FFFFFFFF
      07333FF7F3FFFF3F7FFFBBB0F0000F0F0BB37777F7777373777F3BB0FFFFFFFF
      0BBB3777F3FF3FFF77773330F00F000003333337F773777773333330FFFF0FF0
      33333337F3FF7F37F3333330F08F0F0B33333337F7737F77FF333330FFFF003B
      B3333337FFFF77377FF333B000000333BB33337777777F3377FF3BB3333BB333
      3BB33773333773333773B333333B3333333B7333333733333337}
    NumGlyphs = 2
  end
  object EditResult: TEdit
    Left = 120
    Top = 160
    Width = 201
    Height = 24
    TabOrder = 10
    Text = 'result.txt'
  end
  object BitBtnResult: TBitBtn
    Left = 328
    Top = 160
    Width = 75
    Height = 25
    Caption = '瀏覽'
    TabOrder = 11
    OnClick = BitBtnResultClick
    Glyph.Data = {
      76010000424D7601000000000000760000002800000020000000100000000100
      04000000000000010000130B0000130B00001000000000000000000000000000
      800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
      FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF0033333333B333
      333B33FF33337F3333F73BB3777BB7777BB3377FFFF77FFFF77333B000000000
      0B3333777777777777333330FFFFFFFF07333337F33333337F333330FFFFFFFF
      07333337F3FF3FFF7F333330F00F000F07333337F77377737F333330FFFFFFFF
      07333FF7F3FFFF3F7FFFBBB0F0000F0F0BB37777F7777373777F3BB0FFFFFFFF
      0BBB3777F3FF3FFF77773330F00F000003333337F773777773333330FFFF0FF0
      33333337F3FF7F37F3333330F08F0F0B33333337F7737F77FF333330FFFF003B
      B3333337FFFF77377FF333B000000333BB33337777777F3377FF3BB3333BB333
      3BB33773333773333773B333333B3333333B7333333733333337}
    NumGlyphs = 2
  end
  object EditStandard: TEdit
    Left = 120
    Top = 104
    Width = 201
    Height = 24
    TabOrder = 12
    Text = 'standard.txt'
  end
  object EditDenylist: TEdit
    Left = 120
    Top = 128
    Width = 201
    Height = 24
    TabOrder = 13
    Text = 'denylist.txt'
  end
  object BitBtnStandard: TBitBtn
    Left = 328
    Top = 104
    Width = 75
    Height = 25
    Caption = '瀏覽'
    TabOrder = 14
    OnClick = BitBtnStandardClick
    Glyph.Data = {
      76010000424D7601000000000000760000002800000020000000100000000100
      04000000000000010000130B0000130B00001000000000000000000000000000
      800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
      FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF0033333333B333
      333B33FF33337F3333F73BB3777BB7777BB3377FFFF77FFFF77333B000000000
      0B3333777777777777333330FFFFFFFF07333337F33333337F333330FFFFFFFF
      07333337F3FF3FFF7F333330F00F000F07333337F77377737F333330FFFFFFFF
      07333FF7F3FFFF3F7FFFBBB0F0000F0F0BB37777F7777373777F3BB0FFFFFFFF
      0BBB3777F3FF3FFF77773330F00F000003333337F773777773333330FFFF0FF0
      33333337F3FF7F37F3333330F08F0F0B33333337F7737F77FF333330FFFF003B
      B3333337FFFF77377FF333B000000333BB33337777777F3377FF3BB3333BB333
      3BB33773333773333773B333333B3333333B7333333733333337}
    NumGlyphs = 2
  end
  object BitBtnDenylist: TBitBtn
    Left = 328
    Top = 128
    Width = 75
    Height = 25
    Caption = '瀏覽'
    TabOrder = 15
    OnClick = BitBtnDenylistClick
    Glyph.Data = {
      76010000424D7601000000000000760000002800000020000000100000000100
      04000000000000010000130B0000130B00001000000000000000000000000000
      800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
      FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF0033333333B333
      333B33FF33337F3333F73BB3777BB7777BB3377FFFF77FFFF77333B000000000
      0B3333777777777777333330FFFFFFFF07333337F33333337F333330FFFFFFFF
      07333337F3FF3FFF7F333330F00F000F07333337F77377737F333330FFFFFFFF
      07333FF7F3FFFF3F7FFFBBB0F0000F0F0BB37777F7777373777F3BB0FFFFFFFF
      0BBB3777F3FF3FFF77773330F00F000003333337F773777773333330FFFF0FF0
      33333337F3FF7F37F3333330F08F0F0B33333337F7737F77FF333330FFFF003B
      B3333337FFFF77377FF333B000000333BB33337777777F3377FF3BB3333BB333
      3BB33773333773333773B333333B3333333B7333333733333337}
    NumGlyphs = 2
  end
  object OpenDialogWish: TOpenDialog
    Filter = 
      'Wishes text files (wish*.txt)|wish*.txt|Test files (*.txt)|*.txt' +
      '|All files (*.*)|*.*'
    Left = 8
    Top = 224
  end
  object OpenDialogStdinfo: TOpenDialog
    Filter = 
      'Student Info text files (*.txt)|stdinfo*.txt|Text files (*.txt)|' +
      '*.txt|All files (*.*)|*.*'
    Left = 40
    Top = 224
  end
  object OpenDialogScore: TOpenDialog
    Filter = 
      'Student score text files (score*.txt)|score*.txt|Text files (*.t' +
      'xt)|*.txt|All files (*.*)|*.*'
    Left = 72
    Top = 224
  end
  object OpenDialogDeplim: TOpenDialog
    Filter = 
      'Department Limit text files (deplim*.txt)|deplim*.txt|Text files' +
      ' (*.txt)|*.txt|All files (*.*)|*.*'
    Left = 104
    Top = 224
  end
  object OpenDialogStandard: TOpenDialog
    Filter = 
      'Standard text files (standard*.txt)|standard*.txt|Text files (*.' +
      'txt)|*.txt|All files (*.*)|*.*'
    Left = 136
    Top = 224
  end
  object OpenDialogDenylist: TOpenDialog
    Filter = 
      'DenyList text files (denylist*.txt)|denylist*.txt|Text files (*.' +
      'txt)|*.txt|All files (*.*)|*.*'
    Left = 168
    Top = 224
  end
  object OpenDialogResult: TOpenDialog
    Filter = 'Text files (*.txt)|*.txt|All files (*.*)|*.*'
    Left = 200
    Top = 224
  end
end
