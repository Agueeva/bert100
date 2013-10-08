# -*- coding: utf-8 -*- 

###########################################################################
## Python code generated with wxFormBuilder (version Jun 30 2011)
## http://www.wxformbuilder.org/
##
## PLEASE DO "NOT" EDIT THIS FILE!
###########################################################################

import wx
import wx.xrc

###########################################################################
## Class GEN
###########################################################################

class GEN ( wx.Panel ):

  def __init__( self, parent ):
    #wx.Panel.__init__ ( self, parent, id = wx.ID_ANY, pos = wx.DefaultPosition, size = wx.Size( 250,75 ), style = 0 )
    wx.Panel.__init__ ( self, parent, id = wx.ID_ANY, pos = wx.DefaultPosition, style = 0 )
		
    bSize = wx.BoxSizer( wx.VERTICAL )
    #bSize.SetMinSize( wx.Size( 294,97 ) ) 
    
    self.staticBox = wx.StaticBox( self, wx.ID_ANY, u"Tx" )   
    sbSizer0 = wx.StaticBoxSizer( self.staticBox, wx.VERTICAL )

    
    #self.staticBox = wx.StaticBox( self, wx.ID_ANY) #GJL
    #sbSizer0 = wx.StaticBoxSizer(self.staticBox, wx.VERTICAL ) #GJL
    #sbSizer0 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Rx" ), wx.VERTICAL )    
    
    hSizer00 = wx.BoxSizer( wx.HORIZONTAL )
		
    self.m_staticText31 = wx.StaticText( self, wx.ID_ANY, u"Pattern", wx.DefaultPosition, wx.Size( 50,-1 ), 0 )
    self.m_staticText31.Wrap( -1 )
    self.m_staticText31.SetFont( wx.Font( 8, 74, 90, 92, False, "Tahoma" ) )
    
    hSizer00.Add( self.m_staticText31, 0, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 5 )
		
    self.m_pattern = wx.StaticText( self, wx.ID_ANY, u"PRBS31", wx.DefaultPosition, wx.Size( 60,-1 ), wx.ALIGN_CENTRE|wx.ST_NO_AUTORESIZE|wx.RAISED_BORDER )
    self.m_pattern.Wrap( -1 )
    hSizer00.Add( self.m_pattern, 0, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 0 )
		
    self.m_cdr = wx.StaticText( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.Size( 30,-1 ), wx.ALIGN_CENTRE|wx.ST_NO_AUTORESIZE|wx.RAISED_BORDER )
    self.m_cdr.Wrap( -1 )
    hSizer00.Add( self.m_cdr, 0, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 0 )
		
    self.m_saturate = wx.StaticText( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.Size( 60,-1 ), wx.ALIGN_CENTRE|wx.ST_NO_AUTORESIZE|wx.RAISED_BORDER )
    self.m_saturate.Wrap( -1 )
    hSizer00.Add( self.m_saturate, 0, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 0 )
		
		
    hSizer00.AddSpacer( ( 10, 0), 0, wx.EXPAND, 5 )
		
    sbSizer0.Add( hSizer00, 0, 0, 0 )
		
    hSizer01 = wx.BoxSizer( wx.HORIZONTAL )
		
    self.m_staticText34 = wx.StaticText( self, wx.ID_ANY, u"Post", wx.DefaultPosition, wx.DefaultSize, 0 )
    self.m_staticText34.Wrap( -1 )
    hSizer01.Add( self.m_staticText34, 0, wx.ALL|wx.ALIGN_CENTER_HORIZONTAL|wx.ALIGN_CENTER_VERTICAL, 1 )
		
    self.m_post = wx.SpinCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.Size( 50,-1 ), wx.SP_ARROW_KEYS, 0, 7, 0 )
    hSizer01.Add( self.m_post, 0, wx.ALL|wx.ALIGN_RIGHT|wx.ALIGN_CENTER_VERTICAL, 1 )
		
    self.m_staticText35 = wx.StaticText( self, wx.ID_ANY, u"Pre", wx.DefaultPosition, wx.DefaultSize, 0 )
    self.m_staticText35.Wrap( -1 )
    hSizer01.Add( self.m_staticText35, 0, wx.ALL|wx.ALIGN_CENTER_HORIZONTAL|wx.ALIGN_CENTER_VERTICAL, 1 )
		
    self.m_pre = wx.SpinCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.Size( 50,-1 ), wx.SP_ARROW_KEYS, 0, 7, 0 )
    hSizer01.Add( self.m_pre, 0, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 1 )
		
    self.m_staticText36 = wx.StaticText( self, wx.ID_ANY, u"Swing", wx.DefaultPosition, wx.DefaultSize, 0 )
    self.m_staticText36.Wrap( -1 )
    hSizer01.Add( self.m_staticText36, 0, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 1 )
		
    self.m_swing = wx.SpinCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.Size( 50,-1 ), wx.SP_ARROW_KEYS, 0, 7, 0 )
    hSizer01.Add( self.m_swing, 0, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 1 )
		
    sbSizer0.Add( hSizer01, 1, wx.EXPAND, 5 )
		
    bSize.Add( sbSizer0, 0, 0, 0 )
		
    self.SetSizer( bSize )
    self.Layout()
	    

