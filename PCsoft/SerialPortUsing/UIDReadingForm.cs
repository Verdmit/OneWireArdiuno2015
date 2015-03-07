﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using SerialPortUsing.Properties;

namespace SerialPortUsing
{
	public partial class UIDReadingForm : Form
	{
		private UIDCOMListener _listener;
		public string ResultingUID;
		public UIDReadingForm(){
			_listener = new UIDCOMListener(Settings.Default.COMName, Settings.Default.BaudRate, 8, ReceivingHandler);
			InitializeComponent();
		}

		private void ReceivingHandler(object sender, string uid)
		{
			_listener.Close();
			ReturnUID(uid);
		}

		private delegate void CloseCallBack(string uid);
		private void ReturnUID(string uid)
		{
			if (this.InvokeRequired)
			{
				CloseCallBack cb = ReturnUID;
				Invoke(cb, uid);
			}
			else
			{
				this.DialogResult = DialogResult.OK;
				ResultingUID = uid;
				Close();
			}
		}

		private void b_cancel_Click(object sender, EventArgs e)
		{
			_listener.Close();
			DialogResult = DialogResult.Cancel;
			this.Close();
		}
	}
}
