defmodule ElixirAleRpiTest do
  use ExUnit.Case

  test "dth11h" do
    {:ok, pid} = GpioRpi.start_link 4, :output
    result = GpioRpi.dht11 pid
    assert result == :ok
  end

  test "start_link as :output" do
    IO.puts "*************test ouput"
    {status, pid} = GpioRpi.start_link 4, :output
    assert status == :ok
  end
end
