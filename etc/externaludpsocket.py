#!/usr/bin/env python 

import wx
from socket import *

class UdpController(wx.App):

	def __init__(self):
		wx.App.__init__(self, 0);
		self._socket = socket(AF_INET, SOCK_DGRAM)
		self._socket.connect(('127.0.0.1', 4444))

	def OnInit(self):
		frame = wx.Frame(None, -1, 'UDP Controller', size=(300, 90))
		frame.Show()
		self.SetTopWindow(frame)
		self.b_btn1 = wx.Button(frame, 1, label="1", pos=(5, 30))
		self.b_btn2 = wx.Button(frame, 2, label="2", pos=(105, 30))
		self.b_btn3 = wx.Button(frame, 3, label="3", pos=(205, 30))

		self.Bind(wx.EVT_BUTTON, self.btnpress1, id=1)
		self.Bind(wx.EVT_BUTTON, self.btnpress2, id=2)
		self.Bind(wx.EVT_BUTTON, self.btnpress3, id=3)
		return True

	def btnpress1(self, event):
		self._socket.send("1")

	def btnpress2(self, event):
		self._socket.send("2")
	
	def btnpress3(self, event):
		self._socket.send("3")

UdpController().MainLoop()

